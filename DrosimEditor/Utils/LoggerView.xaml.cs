using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace DrosimEditor.Utils
{
    /// <summary>
    /// Interaction logic for LoggerView.xaml
    /// </summary>
    public partial class LoggerView : UserControl
    {
        public LoggerView()
        {
            InitializeComponent();
            Loaded += (s, e) =>
            {
                Logger.Log(MessageType.Info, "LoggerView initialized");
                Logger.Log(MessageType.Warn, "LoggerView initialized");
                Logger.Log(MessageType.Error, "LoggerView initialized");

            };
        }

        private void OnClearButtonClick(object sender, RoutedEventArgs e)
        {
            Logger.Clear();

        }

        private void OnMessageFilterButtonClick(object sender, RoutedEventArgs e)
        {
            var filter = 0x0;
            if (toggleInfo.IsChecked == true) filter |= (int)MessageType.Info;
            if (toggleWarn.IsChecked == true) filter |= (int)MessageType.Warn;
            if (toggleError.IsChecked == true) filter |= (int)MessageType.Error;

            Logger.SetMessageFilter(filter);
        }
    }
}
