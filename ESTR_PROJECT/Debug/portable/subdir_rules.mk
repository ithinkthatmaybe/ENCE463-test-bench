################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
portable/heap_2.obj: ../portable/heap_2.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="portable/heap_2.pp" --obj_directory="portable" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

portable/port.obj: ../portable/port.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="portable/port.pp" --obj_directory="portable" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

portable/portasm.obj: ../portable/portasm.asm $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="portable/portasm.pp" --obj_directory="portable" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


