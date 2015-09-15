################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
UART.obj: ../UART.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="UART.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

list.obj: ../list.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="list.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

queue.obj: ../queue.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="queue.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

rit128x96x4.obj: ../rit128x96x4.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="rit128x96x4.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

startup_ccs.obj: ../startup_ccs.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="startup_ccs.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

stopwatch.obj: ../stopwatch.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="stopwatch.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

tasks.obj: ../tasks.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="tasks.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

uut_gpio.obj: ../uut_gpio.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M3 --abi=eabi -me -g --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="J:/courses/ENEL463/StellarisWare/utils" --include_path="J:/courses/ENEL463/StellarisWare/boards/ek-lm3s1968" --include_path="J:/courses/ENEL463/StellarisWare" --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="uut_gpio.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


