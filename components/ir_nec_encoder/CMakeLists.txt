set(component_srcs "ir_nec_encoder.c")

idf_component_register(
	SRCS "${component_srcs}"
	PRIV_REQUIRES driver esp_driver_rmt
	INCLUDE_DIRS "."
)
