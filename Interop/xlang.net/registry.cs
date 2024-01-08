using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace xlang.net;

public class ObjectRegistry
{
    private Dictionary<IntPtr, object> idToObject = new Dictionary<IntPtr, object>();
    private Dictionary<object, IntPtr> objectToId = new Dictionary<object, IntPtr>();
    private long nextId = 1;  // Starting ID
    public IntPtr RegisterObject(object instance)
    {
        if (instance == null)
        {
            throw new ArgumentNullException(nameof(instance), "Instance cannot be null.");
        }

        // Check if the object is already registered
        if (objectToId.TryGetValue(instance, out var existingId))
        {
            return existingId; // Return the existing ID if the object is already registered
        }

        // Create a new ID and register the object
        IntPtr newId = new IntPtr(nextId++);
        idToObject[newId] = instance;
        objectToId[instance] = newId;

        return newId;
    }


    public IntPtr GetIdForObject(object instance)
    {
        if (instance == null)
        {
            throw new ArgumentNullException(nameof(instance), "Instance cannot be null.");
        }

        if (objectToId.TryGetValue(instance, out var id))
        {
            return id;
        }

        return IntPtr.Zero; // Or some other way to indicate 'not found'
    }

    public object? GetObject(IntPtr id)
    {
        if (idToObject.TryGetValue(id, out var instance))
        {
            return instance;
        }

        return null; // Or some other way to indicate 'not found'
    }

    public void UnregisterObject(IntPtr id)
    {
        idToObject.Remove(id);
    }
}
