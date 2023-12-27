using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace xlang.net;
public class ObjectRegistry
{
    private Dictionary<IntPtr, object> registry = new Dictionary<IntPtr, object>();
    private long nextId = 1;  // Starting ID

    public IntPtr RegisterObject(object obj)
    {
        IntPtr id = new IntPtr(nextId++);
        registry[id] = obj;
        return id;
    }

    public object GetObject(IntPtr id)
    {
        if (registry.TryGetValue(id, out object obj))
        {
            return obj;
        }
        return null;
    }

    public void UnregisterObject(IntPtr id)
    {
        registry.Remove(id);
    }
}
