install(
    TARGETS bloomers_exe
    RUNTIME COMPONENT bloomers_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
