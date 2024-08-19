project ("xlang_pico")

add_definitions(-DBARE_METAL)

include_directories("../..")
include_directories("../../Api")
include_directories("../../Inc")
include_directories("../../Main")
include_directories("../../utils")
include_directories("../../Ast")
include_directories("../../Parse")
include_directories("../../Serialize")
include_directories("../../rpc")
include_directories("../../Thread")
include_directories("../../Ext")
include_directories("../../Core")
include_directories("../../Text")
include_directories("../../Scripts")
include_directories("../../Tensor")
include_directories("../../Executor")
include_directories("../../Jit")


FILE(GLOB Api_SRC "../../Api/value.cpp")
FILE(GLOB Utils_SRC "../../utils/wait_bare_metal.cpp" "../../utils/gthread_bare_metal.cpp" "../../utils/utility.cpp")
FILE(GLOB Core_SRC "../../Core/*.cpp")
FILE(GLOB Ast_SRC "../../Ast/*.cpp")
FILE(GLOB Serialize_SRC "../../Serialize/*.cpp")
FILE(GLOB Parse_SRC "../../Parse/*.cpp")
FILE(GLOB Thread_SRC "../../Thread/*.cpp")
FILE(GLOB Ext_SRC "../../Ext/event.cpp")
FILE(GLOB Text_SRC "../../Text/*.cpp")
FILE(GLOB Rpc_SRC "../../rpc/*.cpp")
FILE(GLOB Nn_SRC "../../Tensor/*.cpp")
FILE(GLOB Jit_SRC "../../Jit/*.cpp")
FILE(GLOB Main_SRC "../../Main/AddScripts.cpp" "../../Main/Hosting.cpp" "../../Main/xhost_impl.cpp")

FILE(GLOB AppEntry_SRC "../../cli.cpp")
FILE(GLOB Pico_SRC "./*.cpp")

#Add xlang os module into this standalone app

FILE(GLOB Os_SRC "../../os/fs.cpp")


add_executable (${PROJECT_NAME}
	${Api_SRC}
	${Utils_SRC}
	${Core_SRC}
	${Ast_SRC}
	${Parse_SRC}
	${Serialize_SRC}
#	${Thread_SRC}
	${Ext_SRC}
#	${Text_SRC}
#	${Rpc_SRC}
#	${Nn_SRC}
	${Main_SRC}
#	${AppEntry_SRC}
#	${Jit_SRC}
	${Pico_SRC}
#	${Os_SRC}
)

# pull in common dependencies
target_link_libraries(xlang_pico pico_stdlib)

set_source_files_properties(	
	${Api_SRC}
	${Utils_SRC}
	${Core_SRC}
	${Ast_SRC}
	${Parse_SRC}
	${Serialize_SRC}
	#	${Thread_SRC}
	${Ext_SRC}
	#	${Text_SRC}
	#	${Rpc_SRC}
	#	${Nn_SRC}
	${Main_SRC}
	#	${AppEntry_SRC}
	#	${Jit_SRC}
	${Pico_SRC}
	#	${Os_SRC}
	PROPERTIES COMPILE_OPTIONS "-frtti")

# create map/bin/hex file etc.
pico_add_extra_outputs(xlang_pico)
