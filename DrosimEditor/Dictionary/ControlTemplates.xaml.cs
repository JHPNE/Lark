using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;

namespace DrosimEditor.Dictionary
{
    public partial class ControlTemplates : ResourceDictionary
    {
        private void OnTextBoxKeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {

            var textBox = sender as TextBox;
            var exp = textBox.GetBindingExpression(TextBox.TextProperty);
            if (exp == null) return;

            if (e.Key == Key.Enter)
            {
                if (textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
                {
                    command.Execute(textBox.Text);
                }
                else
                {
                    exp.UpdateSource();
                }
                Keyboard.ClearFocus();
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                exp.UpdateTarget();
                Keyboard.ClearFocus();
            }
        }

        private void OnCloseButtonClick(object sender, RoutedEventArgs e)
        {
            var window = (Window) ((FrameworkElement)sender).TemplatedParent;
            window.Close();
        }

        private void OnMaximizeRestoreButtonClick(object sender, RoutedEventArgs e)
        {
            var window = (Window) ((FrameworkElement)sender).TemplatedParent;
            window.WindowState = window.WindowState == WindowState.Normal ? WindowState.Maximized : WindowState.Normal;
        }

        private void OnMinimizeButtonClick(object sender, RoutedEventArgs e)
        {
            var window = (Window) ((FrameworkElement)sender).TemplatedParent;
            window.WindowState = WindowState.Minimized;

        }
    }
}
