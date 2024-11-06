using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace DrosimEditor.Utils.Controls
{
    [TemplatePart(Name = "PART_textBlock", Type = typeof(TextBlock))]
    [TemplatePart(Name = "PART_textBox", Type = typeof(TextBox))]
    public class NumberBox : Control
    {
        private double _originalValue;
        private bool _valueChanged;
        private double _multiplier;
        private bool _captured = false;
        private double _mouseXStart;
        public double Multiplier 
        {
            get => (double)GetValue(MultiplierProperty);
            set => SetValue(MultiplierProperty, value);
        }

        public static readonly DependencyProperty MultiplierProperty =
            DependencyProperty.Register(nameof(Multiplier), typeof(double), typeof(NumberBox), new FrameworkPropertyMetadata(1.0));


        public string Value
        {
            get => (string)GetValue(ValueProperty);
            set => SetValue(ValueProperty, value);
        }

        public static readonly DependencyProperty ValueProperty =
            DependencyProperty.Register(nameof(Value), typeof(string), typeof(NumberBox), new FrameworkPropertyMetadata("0", FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        static NumberBox()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(NumberBox),
                new FrameworkPropertyMetadata(typeof(NumberBox)));
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            if (GetTemplateChild("PART_textBlock") is TextBlock textBlock)
            {
                textBlock.MouseLeftButtonDown += OnTextBlock_Mouse_LBD;
                textBlock.MouseLeftButtonUp += OnTextBlock_Mouse_LBU;
                textBlock.MouseMove += OnTextBlock_Mouse_Move;
            }

            if (GetTemplateChild("PART_textBox") is TextBox textBox)
            {
                textBox.LostFocus += OnTextBox_LostFocus;
                textBox.KeyDown += OnTextBox_KeyDown;
            }
        }

        private void OnTextBlock_Mouse_Move(object sender, MouseEventArgs e)
        {
            if (_captured)
            {
                var mouseX = e.GetPosition(this).X;
                var d = mouseX - _mouseXStart;
                if (Math.Abs(d) > SystemParameters.MinimumHorizontalDragDistance)
                {
                    if (double.TryParse(Value, NumberStyles.Float, CultureInfo.InvariantCulture, out var currentValue))
                    {
                        if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control)) _multiplier = 0.001;
                        else if (Keyboard.Modifiers.HasFlag(ModifierKeys.Shift)) _multiplier = 0.1;
                        else _multiplier = 0.01;

                        var newValue = _originalValue + (d * _multiplier * Multiplier);
                        Value = newValue.ToString("0.#####", CultureInfo.InvariantCulture);
                        _valueChanged = true;
                    }
                }
            }
        }

        private void OnTextBlock_Mouse_LBU(object sender, MouseButtonEventArgs e)
        {
            if (_captured)
            {
                Mouse.Capture(null);
                _captured = false;
                e.Handled = true;
                if (!_valueChanged && GetTemplateChild("PART_textBox") is TextBox textBox)
                {
                    textBox.Visibility = Visibility.Visible;
                    textBox.Focus();
                    textBox.SelectAll();
                }
            }
        }

        private void OnTextBlock_Mouse_LBD(object sender, MouseButtonEventArgs e)
        {
            if (double.TryParse(Value, NumberStyles.Float, CultureInfo.InvariantCulture, out _originalValue))
            {
                Mouse.Capture(sender as UIElement);
                _captured = true;
                _valueChanged = false;
                e.Handled = true;

                _multiplier = 0.01;
                _mouseXStart = e.GetPosition(this).X;
                Focus();
            }
        }

        private void OnTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (GetTemplateChild("PART_textBox") is TextBox textBox)
            {
                textBox.Visibility = Visibility.Collapsed;
            }
        }

        private void OnTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                if (GetTemplateChild("PART_textBox") is TextBox textBox)
                {
                    Value = textBox.Text;
                    textBox.Visibility = Visibility.Collapsed;
                }
            }
            else if (e.Key == Key.Escape)
            {
                if (GetTemplateChild("PART_textBox") is TextBox textBox)
                {
                    textBox.Visibility = Visibility.Collapsed;
                }
            }
        }
    }
}
