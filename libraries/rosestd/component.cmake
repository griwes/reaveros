set(REAVEROS_COMPONENT_ARCHITECTURES ${REAVEROS_ARCHITECTURES})
set(REAVEROS_COMPONENT_INSTALL_PATH [=[sysroots/${_architecture}-${_mode}]=])
set(REAVEROS_COMPONENT_MODES freestanding hosted tests)
set(REAVEROS_COMPONENT_DEPENDS [=[library-rosert-${_mode}-${_architecture}]=])
