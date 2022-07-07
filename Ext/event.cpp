#include "event.h"
namespace X
{
	void Event::FireInMain()
	{
		EventSystem::I().FireInMain(this);
	}
}