using Microsoft.VisualBasic;
using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace X.Net;

public enum BridgeCallType : int
{
    CreateClass,
    CallFunction,
}
public enum PackageMemberType : int
{
    Func,
    FuncEx,
    Prop,
    Const,
    ObjectEvent,
    Class,
    ClassInstance,
}
public enum ValueType : int
{
    Invalid,
	None,
	Int64,
	Double,
	Object,
	Str,
	Value,
};
[StructLayout(LayoutKind.Explicit)]
public struct Value
{
    [FieldOffset(0)]
    public int flags;

    [FieldOffset(4)]
    public ValueType t;

    [FieldOffset(8)]
    public long l;

    [FieldOffset(8)]
    public double d;

    [FieldOffset(8)]
    public IntPtr str; // Use IntPtr for string

    [FieldOffset(8)]
    public IntPtr obj; // Use IntPtr for Object
}


[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void BridgeCallbackDelegate(BridgeCallType callType, int param);

public class XObj : IConvertible
{
    private XLangEng xLangEng;
    public IntPtr xObjPtr;

    public XObj(XLangEng eng,IntPtr ptr)
    {
        xLangEng = eng;
        xObjPtr = ptr;
    }

    public TypeCode GetTypeCode()
    {
        throw new NotImplementedException();
    }

    public bool ToBoolean(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public byte ToByte(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public char ToChar(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public DateTime ToDateTime(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public decimal ToDecimal(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public double ToDouble(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public short ToInt16(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public int ToInt32(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public long ToInt64(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public sbyte ToSByte(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public float ToSingle(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public string ToString(IFormatProvider? provider)
    {
        var str = xLangEng.callObjectToString(xObjPtr);
        if(str!=IntPtr.Zero)
        {
            string? retStr =  Marshal.PtrToStringAnsi(str);
            xLangEng.releaseString(str);
            if (retStr != null)
            {
                return retStr;
            }
            else
            {
                return "";
            }
        }
        else
        {
            return "";
        }
    }

    public object ToType(Type conversionType, IFormatProvider? provider)
    {
        if(conversionType == typeof(Byte[]))
        {
            long size = xLangEng.getObjectBinaryData(xObjPtr, out IntPtr dataPtr);
            if(size > 0)
            {
                byte[] data = new byte[size];
                Marshal.Copy(dataPtr, data, 0, (int)size);
                return data;
            }
            else
            {
                return null;
            }
        }
        return null;
    }

    public ushort ToUInt16(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public uint ToUInt32(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }

    public ulong ToUInt64(IFormatProvider? provider)
    {
        throw new NotImplementedException();
    }
}

public class XLangEng
{
    private const string DllName = "D:\\ToGithub\\CantorAI\\out\\build\\x64-debug\\bin\\xlang_interop.dll"; // or "xlang_eng.so" for Linux
    //private const string DllName = "C:\\ToGithub\\CantorAI\\xlang\\out\\build\\x64-Debug\\bin\\xlang_interop.dll";
    private IntPtr xlangContext = IntPtr.Zero;
    private xlang.net.ObjectRegistry objectRegistry = new xlang.net.ObjectRegistry();

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr LoadLibrary(string dllToLoad);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool FreeLibrary(IntPtr hModule);

    private IntPtr hModule;

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr CreateOrGetClassInstanceDelegate(IntPtr createFuncOrInstance);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool InvokeMethodDelegate(IntPtr classInstance, string methodName, 
        IntPtr variantArray, int arrayLength, out Value returnValue);


    private delegate void LoadDelegate(
        CreateOrGetClassInstanceDelegate cbCreate,
        InvokeMethodDelegate cbInvoke,
        out IntPtr ppContext);
    private delegate void UnloadDelegate(IntPtr pContext);
    private delegate bool CreateAPISetDelegate(out IntPtr ppApiSet);
    private delegate bool AddApiDelegate(IntPtr pVoidApiSet, PackageMemberType type, string name);
    private delegate void DestroyAPISetDelegate(IntPtr pApiSet);
    private delegate bool RegisterPackageDelegate(string className,
         IntPtr pVoidApiSet, bool singleInstance, IntPtr createFuncOrInstance);
    public delegate bool CallObjectFuncDelegate(IntPtr pObjPtr, string funcName,
                     IntPtr variantArray, int arrayLength, out Value returnValue);
    public delegate bool FireObjectEventDelegate(IntPtr objPtr, int evtId,IntPtr variantArray, int arrayLength);
    public delegate IntPtr CallObjectToStringDelegate(IntPtr pObjPtr);
    public delegate void ReleaseStringDelegate(IntPtr str);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate long GetObjectBinaryDataDelegate(IntPtr objectHandle, out IntPtr dataPtr);

    public delegate object CreateClassInstanceDelegate();

    //for Module
    public delegate bool LoadXModuleDelegate(string modulePath, string xlangCode, int size, out IntPtr ppModule);
    public delegate bool RunXModuleDelegate(IntPtr module,out Value returnValue);
    public delegate bool UnloadXModuleDelegate(IntPtr module);


    private LoadDelegate load;
    private UnloadDelegate unload;
    private CreateAPISetDelegate createAPISet;
    private AddApiDelegate addApi;
    private DestroyAPISetDelegate destroyAPISet;
    private RegisterPackageDelegate registerPackage;
    public CallObjectFuncDelegate callObjectFunc;
    public CallObjectToStringDelegate callObjectToString;
    public GetObjectBinaryDataDelegate getObjectBinaryData;
    public ReleaseStringDelegate releaseString;
    public FireObjectEventDelegate fireObjectEvent;
    public LoadXModuleDelegate loadXModule;
    public UnloadXModuleDelegate unloadXModule;
    public RunXModuleDelegate runXModule;

    private object[] ConvertValueArray(IntPtr variantsPtr, int size)
    {
        var result = new object[size];
        var variantSize = Marshal.SizeOf(typeof(Value));

        for (int i = 0; i < size; i++)
        {
            IntPtr variantPtr = IntPtr.Add(variantsPtr, variantSize * i);
            var variant = Marshal.PtrToStructure<Value>(variantPtr);

            switch (variant.t)
            {
                case ValueType.Int64:
                    if(variant.flags == 1)
                    {
                        result[i] = (variant.l > 0);
                    }
                    else
                    {
                        result[i] = variant.l;
                    }
                    result[i] = variant.l;
                    break;
                case ValueType.Double:
                    result[i] = variant.d;
                    break;
                case ValueType.Str:
                    result[i] = Marshal.PtrToStringAnsi(variant.str,variant.flags);
                    break;
                case ValueType.Object:
                    result[i] = new XObj(this,variant.obj);
                    break;
            }
        }

        return result;
    }
    public IntPtr ConvertArrayToVariants(object[] managedArray)
    {
        int size = managedArray.Length;
        int variantSize = Marshal.SizeOf(typeof(Value));
        IntPtr variantsPtr = Marshal.AllocHGlobal(variantSize * size);

        for (int i = 0; i < size; i++)
        {
            IntPtr variantPtr = IntPtr.Add(variantsPtr, variantSize * i);
            Value variant = ConvertToVariant(managedArray[i]);
            Marshal.StructureToPtr(variant, variantPtr, false);
        }

        return variantsPtr;
    }

    private object[] ConvertValueToParameters(object[] managedArray, ParameterInfo[] parameters)
    {
        if (managedArray.Length != parameters.Length)
        {
            throw new ArgumentException("The number of parameters does not match the method signature.");
        }

        object[] convertedParameters = new object[parameters.Length];

        for (int i = 0; i < parameters.Length; i++)
        {
            var param = managedArray[i];
            if(param == null)
            {
                continue;
            }
            Type paramType = parameters[i].ParameterType;
            convertedParameters[i] = Convert.ChangeType(param,paramType);
        }

        return convertedParameters;
    }

    private Value ConvertToVariant(object? obj)
    {
        Value variant = new Value();
        if(obj == null)
        {
            variant.t = ValueType.None;
            return variant;
        }
        switch (obj)
        {
            case bool boolValue:
                variant.t = ValueType.Int64;
                variant.l = boolValue ? 1 : 0;
                variant.flags = 1;
                break;
            case int intValue:
                variant.t = ValueType.Int64;
                variant.l = intValue;
                break;
            case double doubleValue:
                variant.t = ValueType.Double;
                variant.d = doubleValue;
                break;
            case string stringValue:
                variant.t = ValueType.Str;
                variant.str = Marshal.StringToHGlobalAnsi(stringValue);
                variant.flags = stringValue.Length;
                break;
            case XObj xObjWrapper:
                variant.t = ValueType.Object;
                variant.obj = xObjWrapper.xObjPtr;
                break;
            default:
                variant.t = ValueType.Object;
                variant.obj = GetPointerToObject(obj);
                break;
        }

        return variant;
    }

    public XLangEng()
    {
        //string folder = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

        hModule = LoadLibrary(DllName);
        if (hModule == IntPtr.Zero)
        {
            throw new Exception("Failed to load library: " + DllName);
        }

        load = Marshal.GetDelegateForFunctionPointer<LoadDelegate>(GetProcAddress(hModule, "Load"));
        unload = Marshal.GetDelegateForFunctionPointer<UnloadDelegate>(GetProcAddress(hModule, "Unload"));
        createAPISet = Marshal.GetDelegateForFunctionPointer<CreateAPISetDelegate>(GetProcAddress(hModule, "CreateAPISet"));
        addApi = Marshal.GetDelegateForFunctionPointer<AddApiDelegate>(GetProcAddress(hModule, "AddApi"));
        destroyAPISet = Marshal.GetDelegateForFunctionPointer<DestroyAPISetDelegate>(GetProcAddress(hModule, "DeleteAPISet"));
        registerPackage = Marshal.GetDelegateForFunctionPointer<RegisterPackageDelegate>(GetProcAddress(hModule, "RegisterPackage"));
        callObjectFunc = Marshal.GetDelegateForFunctionPointer<CallObjectFuncDelegate>(GetProcAddress(hModule, "CallObjectFunc"));
        callObjectToString = Marshal.GetDelegateForFunctionPointer<CallObjectToStringDelegate>(GetProcAddress(hModule, "CallObjectToString"));
        releaseString = Marshal.GetDelegateForFunctionPointer<ReleaseStringDelegate>(GetProcAddress(hModule, "ReleaseString"));
        fireObjectEvent = Marshal.GetDelegateForFunctionPointer<FireObjectEventDelegate>(GetProcAddress(hModule, "FireObjectEvent"));
        getObjectBinaryData = Marshal.GetDelegateForFunctionPointer<GetObjectBinaryDataDelegate>(GetProcAddress(hModule, "GetObjectBinaryData"));
        loadXModule = Marshal.GetDelegateForFunctionPointer<LoadXModuleDelegate>(GetProcAddress(hModule, "LoadXModule"));
        unloadXModule = Marshal.GetDelegateForFunctionPointer<UnloadXModuleDelegate>(GetProcAddress(hModule, "UnloadXModule"));
        runXModule = Marshal.GetDelegateForFunctionPointer<RunXModuleDelegate>(GetProcAddress(hModule, "RunXModule"));

    }
    public IntPtr RunXModule(string modulePath,string code, out Value returnValue)
    {
        IntPtr hModule = IntPtr.Zero;
        bool bOK = loadXModule(modulePath,code, code.Length, out hModule);
        if(bOK)
        {
            bOK = runXModule(hModule, out returnValue);
            if (bOK)
            {
                return hModule;
            }
        }
        returnValue = new Value();
        return IntPtr.Zero;
    }
    public void UnloadXModule(IntPtr hModule)
    {
        unloadXModule(hModule);
    }
    public IntPtr GetPointerToObject(object obj)
    {
        return objectRegistry.RegisterObject(obj);
        //GCHandle handle = GCHandle.Alloc(obj/*GCHandleType.Pinned*/);
        //TODO: handle.Free(); need to called later
        //return GCHandle.ToIntPtr(handle);
    }
    public object? GetObjectFromPointer(IntPtr pointer)
    {
        return objectRegistry.GetObject(pointer);
        //GCHandle handle = GCHandle.FromIntPtr(pointer);
        //return handle.Target;
    }
    public bool FireEvent(object obj, int evtId, object[] args)
    {
        IntPtr objPtr = GetPointerToObject(obj);
        IntPtr argsPtr = ConvertArrayToVariants(args);
        return fireObjectEvent(objPtr, evtId, argsPtr, args.Length);
    }
    public object CallFunc(object objFunc,object[] args)
    {
        if(objFunc is XObj xObj)
        {
            IntPtr argsPtr = ConvertArrayToVariants(args);
            Value outVal;
            callObjectFunc(xObj.xObjPtr, "", argsPtr, args.Length, out outVal);
            return outVal;
        }
        //IntPtr objPtr = GetPointerToObject(objFunc);

        return null;
    }
    public IntPtr CreateAPISet()
    {
        IntPtr pApiSet;
        createAPISet(out pApiSet);
        return pApiSet;
    }
    public void AddApiToSet(IntPtr pApiSet, PackageMemberType type, string name)
    {
        addApi(pApiSet, type, name);
    }
    public void RegisterClassWithInstance(string name, IntPtr pApiSet,object instance)
    {
        registerPackage(name, pApiSet, true, GetPointerToObject(instance));
    }
    public void RegisterClass(string name, IntPtr pApiSet, CreateClassInstanceDelegate delFunc)
    {
        var createFunc = Marshal.GetFunctionPointerForDelegate(delFunc);
        registerPackage(name, pApiSet, false, createFunc);
    }
    public void RegisterClass<T>(string[] eventlist,T instance)
    {
        var type = typeof(T);
        var methods = type.GetMethods(BindingFlags.Public | BindingFlags.Instance| BindingFlags.DeclaredOnly);
        var apiSet = CreateAPISet();
        foreach (var evt in eventlist)
        {
            AddApiToSet(apiSet, PackageMemberType.ObjectEvent, evt);
        }
        foreach (var method in methods)
        {
            AddApiToSet(apiSet, PackageMemberType.Func, method.Name);
        }
        if (instance == null)
        {
            CreateClassInstanceDelegate delFunc = () => Activator.CreateInstance(type);
            var createFunc = Marshal.GetFunctionPointerForDelegate(delFunc);
            registerPackage(type.Name, apiSet, false, createFunc);
        }
        else
        {
            var ptr = GetPointerToObject(instance);
            registerPackage(type.Name, apiSet, true, ptr);
        }
    }
    // Method to create or retrieve an instance of a class
    public IntPtr CreateOrGetClassInstance(IntPtr createFuncOrInstance)
    {
        var funcDelegate = Marshal.GetDelegateForFunctionPointer<CreateClassInstanceDelegate>(createFuncOrInstance);
        var instance  = funcDelegate();
        return GetPointerToObject(instance);
    }
    public bool InvokeMethod(IntPtr classInstancePtr, string methodName, 
        IntPtr variantArrayPtr, int arrayLength, out Value returnValue)
    {
        // Convert IntPtr back to a managed object reference
        var classInstance = GetObjectFromPointer(classInstancePtr);
        if(classInstance == null)
        {
            returnValue = new Value();
            return false;
        }

        var methodInfo = classInstance.GetType().GetMethod(methodName);
        if(methodInfo == null)
        {
            returnValue = new Value();
            return false;
        }
        // Convert the variant array to a managed array
        object[] managedArray = ConvertValueArray(variantArrayPtr, arrayLength);
        object[] parameters = ConvertValueToParameters(managedArray,methodInfo.GetParameters());
        // Use reflection to invoke the method on the class instance
        //object result = methodInfo.Invoke(classInstance, parameters);
        object? result = null;
        try
        {
            result = methodInfo.Invoke(classInstance, parameters);
        }
        catch (TargetInvocationException ex)
        {
            // Handle exceptions thrown by the invoked method
            // You can access the original exception using ex.InnerException
            Console.WriteLine($"Exception thrown in invoked method: {ex.InnerException}");
        }
        catch (ArgumentException ex)
        {
            // Handle argument exceptions, e.g., method parameters mismatch
            Console.WriteLine($"Argument exception: {ex.Message}");
        }
        catch (TargetException ex)
        {
            // Handle cases where the method is not valid for the given instance
            Console.WriteLine($"Target exception: {ex.Message}");
        }
        catch (MethodAccessException ex)
        {
            // Handle exceptions related to access restrictions
            Console.WriteLine($"Method access exception: {ex.Message}");
        }
        catch (Exception ex)
        {
            // Handle other exceptions
            Console.WriteLine($"General exception: {ex.Message}");
        }
        returnValue = ConvertToVariant(result);
        return true;
    }
    //keep here to avoid GC 
    private InvokeMethodDelegate _invokeMethodDelegate;
    private CreateOrGetClassInstanceDelegate _callbackDelegate;
    public void Load()
    {
        _callbackDelegate = new CreateOrGetClassInstanceDelegate(CreateOrGetClassInstance);
        _invokeMethodDelegate = new InvokeMethodDelegate(InvokeMethod);
        load(_callbackDelegate, _invokeMethodDelegate,out this.xlangContext);
    }

    public void Unload()
    {
        unload(this.xlangContext);
        FreeLibrary(hModule);
    }
}
