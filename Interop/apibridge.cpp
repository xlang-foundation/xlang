#include "apibridge.h"

namespace X
{
	namespace Interop
	{
		std::unordered_map<void*, ApiBridge*> ApiBridge::mMapApiBridge;
	}
}