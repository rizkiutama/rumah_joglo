################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/imageloader.cpp \
../src/rumah_joglo.cpp \
../src/vec3f.cpp 

OBJS += \
./src/imageloader.o \
./src/rumah_joglo.o \
./src/vec3f.o 

CPP_DEPS += \
./src/imageloader.d \
./src/rumah_joglo.d \
./src/vec3f.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


