using DrosimEditor.Utils;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.Serialization;

namespace DrosimEditor.SimProject
{
    [DataContract]
    public class ProjectTemplate
    {
        [DataMember]
        public string pType { get; set; }
        [DataMember]
        public string pFile { get; set; }
        [DataMember]
        public List<string> pFolders { get; set; }
        public byte[] Icon { get; set; }
        public byte[] Screenshot { get; set; }
        public string IconFilePath {  get; set; }
        public string ScreenshotFilePath {  get; set; }
        public string ProjectFilePath {  get; set; }
    }

    class NewProject : ViewModelBase 
    {
        //TODO: get the path from isntallation location
        private readonly string _templatePath = @"..\..\DrosimEditor\ProjectTemplates";
        private string _projectName = "New Project";
        public string ProjectName
        {
            get => _projectName;
            set
            { 
                if (_projectName != value)
                {
                    _projectName = value;
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }

        private string _projectPath = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\Dronsim\";
        public string ProjectPath 
        {
            get => _projectPath;
            set
            { 
                if (_projectPath != value)
                {
                    _projectPath = value;
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        private ObservableCollection<ProjectTemplate> _projectTemplates = new ObservableCollection<ProjectTemplate>();
        public ReadOnlyObservableCollection<ProjectTemplate> ProjectTemplates
        { get; }
        public NewProject()
        {
            ProjectTemplates = new ReadOnlyObservableCollection<ProjectTemplate>(_projectTemplates);
            try
            {
                var templateList = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templateList != null);
                foreach (var file in templateList)
                {
                    var template = Serializer.FromFile<ProjectTemplate>(file);
                    template.IconFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), "Icon.png"));
                    template.Icon = File.ReadAllBytes(template.IconFilePath);
                    template.ScreenshotFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), "Screenshot.png"));
                    template.Screenshot = File.ReadAllBytes(template.ScreenshotFilePath);
                    template.ProjectFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file), template.pFile));

                    _projectTemplates.Add(template);
                }
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }
        }
    }
}
