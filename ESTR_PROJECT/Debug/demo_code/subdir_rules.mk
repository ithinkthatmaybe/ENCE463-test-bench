################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
demo_code/basic_io.obj: ../demo_code/basic_io.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="demo_code/basic_io.pp" --obj_directory="demo_code" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


