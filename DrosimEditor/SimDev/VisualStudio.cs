using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using DrosimEditor.Utils;
using Microsoft.Win32;
using System.Runtime.InteropServices.ComTypes;
using System.Runtime.InteropServices;
using System.IO;
using DrosimEditor.SimProject;

namespace DrosimEditor.SimDev
{
    enum BuildConfiguration
    {
        Debug,
        DebugEditor,
        Release,
        ReleaseEditor
    }

    static class VisualStudio
    {
        private static EnvDTE80.DTE2 _vsInstance = null;
        private static readonly string? _progID = GetVisualStudioProgID();

        private static readonly string[] _buildConfigurationNames = new string[] {"Debug","DebugEditor","Release","ReleaseEditor"};
        private static readonly ManualResetEventSlim _resetEvent = new ManualResetEventSlim(false);
        private static readonly object _lock = new object();

        public static string GetConfigurationName(BuildConfiguration config) => _buildConfigurationNames[(int)config];
        public static bool BuildSucceeded { get; private set; } = true;
        public static bool BuildDone { get; private set; } = true;

        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(int reserved, out IBindCtx ppbc);

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable pprot);

        private static void CallOnSTAThread(Action action)
        {
            Debug.Assert(action != null);
            var thread = new Thread(() =>
            {
                MessageFilter.Register();
                try { action(); }
                catch (Exception e) { Logger.Log(MessageType.Error, e.Message); }
                finally { MessageFilter.Revoke(); }
            });

            thread.SetApartmentState(ApartmentState.STA);
            thread.Start();
            thread.Join();
        }
        private static void OpenVisualStudioInternal(string solutionPath)
        {
            IRunningObjectTable? rot = null;
            IEnumMoniker? monikerTable = null;
            IBindCtx? bindCtx = null;
            try
            {
                if (_vsInstance == null)
                {
                    var hResult = GetRunningObjectTable(0, out rot);
                    if (hResult < 0 || rot == null) throw new COMException($"GetRunningObjectTable() returned HRESULT: {hResult:X8}");

                    rot.EnumRunning(out monikerTable);
                    monikerTable.Reset();

                    hResult = CreateBindCtx(0, out bindCtx);
                    if (hResult < 0 || bindCtx == null) throw new COMException($"CreateBindCtx() returned HRESULT: {hResult:X8}");

                    IMoniker[] currentMoniker = new IMoniker[1];

                    if (_progID == null)
                    {
                        throw new InvalidOperationException("No compatible version of Visual Studio found.");
                    }

                    while (monikerTable.Next(1, currentMoniker, IntPtr.Zero) == 0)
                    {
                        string name = string.Empty;
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        if (name.Contains(_progID))
                        {
                            hResult = rot.GetObject(currentMoniker[0], out object? obj);
                            if (hResult < 0 || obj == null) throw new COMException($"Running object table's GetObject() returned HRESULT: {hResult:X8}");

                            EnvDTE80.DTE2? dte = obj as EnvDTE80.DTE2;

                            var solutionName = string.Empty;
                            CallOnSTAThread(() => { solutionName = dte.Solution.FullName; });
                            if (solutionName == solutionPath)
                            {
                                _vsInstance = dte;
                                break;
                            }
                        }
                    }

                    if (_vsInstance == null)
                    {
                        Type? visualStudioType = Type.GetTypeFromProgID(_progID, true);
                        _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "Failed to find VisualStudio");
            }
            finally
            {
                if (monikerTable != null) Marshal.ReleaseComObject(monikerTable);
                if (rot != null) Marshal.ReleaseComObject(rot);
                if (bindCtx != null) Marshal.ReleaseComObject(bindCtx);
            }
        }

        private static void CloseVisualStudioInternal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance?.Solution.IsOpen == true)
                {
                    _vsInstance.ExecuteCommand("File.SaveAll");
                    _vsInstance.Solution.Close(true);
                }
                _vsInstance?.Quit();
                _vsInstance = null;

            });
        }

        public static void CloseVisualStudio()
        {
            lock (_lock) { CloseVisualStudioInternal(); }
        }

        private static string? GetVisualStudioProgID()
        {
            // List of known Visual Studio ProgIDs
            var progIDs = new List<string>
            {
                "VisualStudio.DTE.17.0", // Visual Studio 2022
                "VisualStudio.DTE.16.0", // Visual Studio 2019
                "VisualStudio.DTE.15.0", // Visual Studio 2017
                "VisualStudio.DTE.14.0", // Visual Studio 2015
                "VisualStudio.DTE.12.0", // Visual Studio 2013
                "VisualStudio.DTE.11.0", // Visual Studio 2012
                "VisualStudio.DTE.10.0", // Visual Studio 2010
                "VisualStudio.DTE.9.0",  // Visual Studio 2008
                "VisualStudio.DTE.8.0"   // Visual Studio 2005
            };

            foreach (var progID in progIDs)
            {
                try
                {
                    Type? type = Type.GetTypeFromProgID(progID);
                    if (type != null)
                    {
                        return progID;
                    }
                }
                catch
                {
                    // Ignore and try the next ProgID
                }
            }

            return null;
        }

        private static bool AddFilesToSolutionInternal(string solution, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0, "No files to add to solution");
            OpenVisualStudioInternal(solution);
            try
            {
                if (_vsInstance != null)
                {
                    CallOnSTAThread(() =>
                    {
                        if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(solution);
                        else _vsInstance.ExecuteCommand("File.SaveAll");

                        foreach (EnvDTE.Project project in _vsInstance.Solution.Projects)
                        {
                            if (project.UniqueName.Contains(projectName))
                            {
                                foreach (var file in files)
                                {
                                    project.ProjectItems.AddFromFile(file);
                                }
                            }
                        }

                        var cpp = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                        if (!string.IsNullOrEmpty(cpp))
                        {
                            _vsInstance.ItemOperations.OpenFile(cpp, "{7651a703-06e5-11d1-8ebd-00a0c90f26ea}").Visible = true;
                        }
                        _vsInstance.MainWindow.Activate();
                        _vsInstance.MainWindow.Visible = true;
                    });
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to add files to solution {solution}");
                return false;
            }
            return true;
        }

        public static bool AddFilesToSolution(string solution, string projectName, string[] files)
        {
            lock (_lock) { return AddFilesToSolutionInternal(solution, projectName, files); }
        }   

        private static bool IsDebuggingInternal()
        {
            bool result = false;
            CallOnSTAThread(() =>
            {
                result = _vsInstance != null && (_vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
            });

            return result;
        }

        public static bool IsDebugging()
        {
            lock (_lock) { return IsDebuggingInternal(); }
        }

        private static void BuildSolutionInternal(Project project, BuildConfiguration buildConfig, bool showWindow = true)
        {
            if (IsDebuggingInternal())
            {
                Logger.Log(MessageType.Error, "Cannot build solution while debugging");
                return;
            }

            OpenVisualStudioInternal(project.Solution);
            BuildDone = BuildSucceeded = false;

            CallOnSTAThread(() =>
            {
                try
                {
                    _vsInstance.MainWindow.Visible = showWindow;
                    if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(project.Solution);

                    _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildSolutionBegin;
                    _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildSolutionDone;
                }
                catch (Exception ex)
                {
                    Logger.Log(MessageType.Error, $"Error in STA thread initialization: {ex.Message}");
                    throw;
                }
            });

            var configName = GetConfigurationName(buildConfig);

            try
            {
                var directoryPath = Path.Combine($"{project.Path}", $@"x64\{configName}");
                if (!Directory.Exists(directoryPath))
                {
                    Directory.CreateDirectory(directoryPath);
                }

                foreach (var pdbFile in Directory.GetFiles(directoryPath, "*.pdb"))
                {
                    File.Delete(pdbFile);
                }
            }
            catch (Exception ex)
            {
                Logger.Log(MessageType.Error, $"Error while cleaning PDB files: {ex.Message}");
            }

            CallOnSTAThread(() =>
            {
                try
                {
                    _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();
                    _vsInstance.ExecuteCommand("Build.BuildSolution");
                    _resetEvent.Wait();
                    _resetEvent.Reset();
                }
                catch (Exception ex)
                {
                    Logger.Log(MessageType.Error, $"Error during build execution: {ex.Message}");
                    throw;
                }
            });
        }

        public static void BuildSolution(Project project, BuildConfiguration buildConfig, bool showWindow = true)
        {
            lock (_lock) { BuildSolutionInternal(project, buildConfig, showWindow); }
        }

        private static void OnBuildSolutionDone(string project, string projectConfig, string platform, string solutionConfig, bool success)
        {
            if (BuildDone) return;

            if (success) Logger.Log(MessageType.Info, $"Building {projectConfig} configuration succeeded");
            else Logger.Log(MessageType.Error, $"Building {projectConfig} configuration failed");

            BuildDone = true;
            BuildSucceeded = success;
            _resetEvent.Set();
        }

        private static void OnBuildSolutionBegin(string project, string projectConfig, string platform, string solutionConfig)
        {
            if (BuildDone) return;
            Logger.Log(MessageType.Info, $"Building {project} {projectConfig} {platform} {solutionConfig}");
        }

        private static void RunInternal(Project project, BuildConfiguration buildConfig, bool debug)
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance != null && !IsDebuggingInternal() && BuildSucceeded)
                {
                    _vsInstance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");
                }
            });
        }

        private static void StopInternal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance != null && IsDebuggingInternal())
                {
                    _vsInstance.ExecuteCommand("Debug.StopDebugging");
                }

            });
        }
    }

    [ComImport(), Guid("00000016-0000-0000-C000-000000000046"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IOleMessageFilter
    {
        [PreserveSig]
        int HandleInComingCall(int dwCallType, IntPtr hTaskCaller, int dwTickCount, IntPtr lpInterfaceInfo);

        [PreserveSig]
        int RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType);

        [PreserveSig]
        int MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType);
    }

    public class MessageFilter : IOleMessageFilter
    {
        private const int SERVERCALL_IS_HANDLED = 0;
        private const int PENDING_MSG_WAIT_DEF_PROCESS = 2;
        private const int SERVERCALL_RETRY_LATER = 2;

        [DllImport("Ole32.dll")]
        private static extern int CoRegisterMessageFilter(IOleMessageFilter newFilter, out IOleMessageFilter oldFilter);
        public static void Register()
        {
            IOleMessageFilter newFilter = new MessageFilter();
            int hr = CoRegisterMessageFilter(newFilter, out var oldFilter);
            Debug.Assert(hr >= 0, $"CoRegisterMessageFilter failed with error {hr}");
        }

        public static void Revoke()
        {
            int hr = CoRegisterMessageFilter(null!, out var oldFilter);
            Debug.Assert(hr >= 0, $"Unregistering COM IMessageFilter failed with error {hr}");
        }

        int IOleMessageFilter.HandleInComingCall(int dwCallType, IntPtr hTaskCaller, int dwTickCount, IntPtr lpInterfaceInfo)
        {
            return SERVERCALL_IS_HANDLED;
        }

        int IOleMessageFilter.RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType)
        {
            if (dwRejectType == SERVERCALL_RETRY_LATER)
            {
                Debug.WriteLine("COM server busy. Retrying call to EnvDTE interface.");
                return 500;
            }
            return -1;
        }

        int IOleMessageFilter.MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType)
        {
            return 2;
        }
    }
}
