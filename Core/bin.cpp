#include "bin.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<0> _binScope;
		void Binary::Init()
		{
			_binScope.Init();
			{

			}
		}
		void Binary::cleanup()
		{
			_binScope.Clean();
		}
	}
}