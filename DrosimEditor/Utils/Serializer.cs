using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace DrosimEditor.Utils
{
    public static class Serializer
    {
        public static void ToFile<T>(T obj, string path)
        {
            try
            {
                using var fs = new FileStream(path, FileMode.Create);
                var serializer = new DataContractSerializer(typeof(T));
                serializer.WriteObject(fs, obj);
            }
            catch (Exception e)  
            { 
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to serialize {obj} to {path}");
                throw;
            }
        }

        internal static T FromFile<T>(string path)
        {
            try
            {
                using var fs = new FileStream(path, FileMode.Open);
                var serializer = new DataContractSerializer(typeof(T));
                T instance = (T) serializer.ReadObject(fs);

                return instance;
            }
            catch (Exception e)  
            { 
                Debug.WriteLine(e.Message);
                Logger.Log(MessageType.Error, $"Failed to deserialize {path}");
                throw;
            }
        }
    }
}
