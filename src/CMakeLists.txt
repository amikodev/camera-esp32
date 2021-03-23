
set(app_sources
	"main.cpp"
    "Camera.cpp"
)

set(includes
	"."
)

set(reqs
)

set(files
)


idf_component_register(SRCS ${app_sources} INCLUDE_DIRS ${includes} REQUIRES ${reqs} EMBED_FILES ${files})
