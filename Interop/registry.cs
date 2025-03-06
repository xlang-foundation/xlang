/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
