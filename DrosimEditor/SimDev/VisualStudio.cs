using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using DrosimEditor.Utils;
using Microsoft.Win32;
using System.Runtime.InteropServices.ComTypes;
using System.Runtime.InteropServices;
using System.IO;

namespace DrosimEditor.SimDev
{
    static class VisualStudio
    {
        private static EnvDTE80.DTE2 _vsInstance = null;
        private static readonly string _progID = GetVisualStudioProgID();

        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(int reserved, out IBindCtx ppbc);

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable pprot);
        public static void OpenVisualStudio(string solutionPath)
        {
            IRunningObjectTable rot = null;
            IEnumMoniker monikerTable = null;
            IBindCtx bindCtx = null;
            try
            {
                if (_vsInstance == null)
                {
                    var hResult = GetRunningObjectTable(0, out rot);
                    if (hResult < 0 || rot == null) throw new COMException($"GetRunnungObjectTable() returned HRESULT: {hResult:X8}");

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
                            hResult = rot.GetObject(currentMoniker[0], out object obj);
                            if (hResult < 0 || obj == null) throw new COMException($"Running object table's GetObject() returned HRESULT: {hResult:X8}");

                            EnvDTE80.DTE2 dte = obj as EnvDTE80.DTE2;
                            var solutionName = dte.Solution.FullName;
                            if (solutionName == solutionPath)
                            {
                                _vsInstance = dte;
                                break;
                            }

                        }
                    }



                    if (_vsInstance == null)
                    {
                        Type visualStudioType = Type.GetTypeFromProgID(_progID, true);
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

        public static void CloseVisualStudio()
        {
            if (_vsInstance?.Solution.IsOpen == true)
            {
                _vsInstance.ExecuteCommand("File.SaveAll");
                _vsInstance.Solution.Close(true);
            }
            _vsInstance?.Quit();
            _vsInstance = null;
        }

        private static string GetVisualStudioProgID()
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
                    Type type = Type.GetTypeFromProgID(progID);
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

        public static bool AddFilesToSolution(string solution, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0, "No files to add to solution");
            OpenVisualStudio(solution);
            try
            {
                if (_vsInstance != null)
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
                        //_vsInstance.ItemOperations.OpenFile(cpp, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                    }
                    _vsInstance.MainWindow.Activate();
                    _vsInstance.MainWindow.Visible = true;
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
    }
}