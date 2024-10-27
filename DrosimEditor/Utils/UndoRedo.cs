using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DrosimEditor.Utils
{
    public interface IUndoRedo
    {
        string Name { get; }
        void Undo();
        void Redo();
    }

    public class UndoRedoAction : IUndoRedo
    {
        private Action _undoAction;
     
        private Action _redoAction;
        public string Name { get; }

        public void Undo() => _undoAction();
        public void Redo() => _redoAction(); 

        public UndoRedoAction(string name)
        {
            Name = name;
        }

        public UndoRedoAction(Action undo, Action redo, string name)
            : this(name)
        {
            Debug.Assert(undo != null && redo != null);
            _undoAction = undo;
            _redoAction = redo;

        }

        public UndoRedoAction(string property, object instance, object undoValue, object redoValue, string name):
            this(
                () => instance.GetType().GetProperty(property).SetValue(instance, undoValue),
                () => instance.GetType().GetProperty(property).SetValue(instance, redoValue),
                name)
        {

        }
    }

    public class UndoRedo
    {
        private readonly ObservableCollection<IUndoRedo> _undoList = new ObservableCollection<IUndoRedo>();
        private readonly ObservableCollection<IUndoRedo> _redoList = new ObservableCollection<IUndoRedo>();
        public ReadOnlyObservableCollection<IUndoRedo> RedoList { get; }
        public ReadOnlyObservableCollection<IUndoRedo> UndoList { get; }

        private bool _enabledAdd = true;



        public UndoRedo()
        {

            RedoList = new ReadOnlyObservableCollection<IUndoRedo>(_redoList);
            UndoList = new ReadOnlyObservableCollection<IUndoRedo>(_undoList);
        }

        public void Reset()
        {
            _redoList.Clear();
            _undoList.Clear();
        }

        public void Undo()
        {
            if (_undoList.Any())
            {
                var last = _undoList.Last();
                _undoList.RemoveAt(_undoList.Count - 1);
                _enabledAdd = false;
                last.Undo();
                _enabledAdd = true;
                _redoList.Insert(0, last);
            }
        }

        public void Redo()
        {
            if (_redoList.Any())
            {
                var last = _redoList.First();
                _redoList.RemoveAt(0);
                _enabledAdd = false;
                last.Redo();
                _enabledAdd = true;
                _undoList.Add(last);
            }
        }

        public void Add(IUndoRedo undoRedo)
        {
            if (_enabledAdd)
            {
                _undoList.Add(undoRedo);
                _redoList.Clear();
            }
        }
    }
}
