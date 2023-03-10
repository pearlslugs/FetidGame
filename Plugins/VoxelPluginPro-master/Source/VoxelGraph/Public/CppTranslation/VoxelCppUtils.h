// Copyright 2021 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"

struct FVoxelGeneratorPicker;
class FVoxelGeneratorVariableHelper;

template<typename T, typename = void>
struct TVoxelGetCppTypeImpl;

#define GET_CPP_TYPE(Type) \
	template<> \
	struct TVoxelGetCppTypeImpl<Type> \
	{ \
		static FString Get() \
		{ \
			return #Type; \
		} \
	};

	GET_CPP_TYPE(bool);
	GET_CPP_TYPE(float);
	GET_CPP_TYPE(double);
	GET_CPP_TYPE(int32);
	GET_CPP_TYPE(FColor);
	GET_CPP_TYPE(FLinearColor);
	GET_CPP_TYPE(FRotator);
	GET_CPP_TYPE(FString);
	GET_CPP_TYPE(FName);
	GET_CPP_TYPE(FVoxelGeneratorPicker);
	GET_CPP_TYPE(FVoxelGeneratorVariableHelper);

#undef GET_CPP_TYPE

template<typename Enum>
struct TVoxelGetCppTypeImpl<Enum, typename TEnableIf<TIsEnum<Enum>::Value>::Type>
{
	static FString Get()
	{
		const FString String = UEnum::GetValueAsString(static_cast<Enum>(0));

		TArray<FString> Array;
		String.ParseIntoArray(Array, TEXT("::"));
		ensure(Array.Num() == 2);

		return Array[0];
	}
};

template<typename T>
struct TVoxelGetCppTypeImpl<T, typename TEnableIf<TIsDerivedFrom<T, UObject>::IsDerived>::Type>
{
	static FString Get()
	{
		return "U" + T::StaticClass()->GetName();
	}
};

template<typename T>
struct TVoxelGetCppTypeImpl<TWeakObjectPtr<T>>
{
	static FString Get()
	{
		return FString::Printf(TEXT("TWeakObjectPtr<%s>"), *TVoxelGetCppTypeImpl<T>::Get());
	}
};

struct FVoxelCppUtils
{
	template<typename T>
	static typename TEnableIf<!TIsEnum<T>::Value, FString>::Type LexToCpp(T Value) = delete;
	
	static FString LexToCpp(bool Value)
	{
		return LexToString(Value);
	}
	static FString LexToCpp(float Value)
	{
		return FString::SanitizeFloat(Value);
	}
	static FString LexToCpp(double Value)
	{
		return FString::SanitizeFloat(Value);
	}
	static FString LexToCpp(int32 Value)
	{
		return FString::FromInt(Value);
	}
	template<typename Enum>
	static typename TEnableIf<TIsEnum<Enum>::Value, FString>::Type LexToCpp(Enum Value)
	{
		return UEnum::GetValueAsString(Value);
	}
	static FString LexToCpp(const FString& Value)
	{
		return FString::Printf(TEXT("\"%s\""), *Value);
	}
	static FString LexToCpp(FName Value)
	{
		return FString::Printf(TEXT("STATIC_FNAME(\"%s\")"), *Value.ToString());
	}
	static FString LexToCpp(const FRotator& Rotation)
	{
		return FString::Printf(TEXT("FRotator(%f, %f, %f)"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
	}
	static FString LexToCpp(const FColor& Color)
	{
		return FString::Printf(TEXT("FColor(%d, %d, %d, %d)"), Color.R, Color.G, Color.B, Color.A);
	}
	static FString LexToCpp(const FLinearColor& Color)
	{
		return FString::Printf(TEXT("FLinearColor(%f, %f, %f, %f)"), Color.R, Color.G, Color.B, Color.A);
	}

public:
	template<typename T>
	static FString TypeToString()
	{
		return TVoxelGetCppTypeImpl<T>::Get();
	}

public:
	template<typename T>
	static FString ArrayToString(const TArray<T>& Array)
	{
		FString Line = "{";
		for (auto& Name : Array)
		{
			Line += " ";
			Line += LexToCpp(Name);
			Line += ",";
		}
		Line += " }";
		return Line;
	}

	template<typename T>
	static void CreateMapString(T& Constructor, const FString& MapName, const TArray<FName>& Keys, const TArray<FString>& Values, int32 ValuesOffset)
	{
		Constructor.StartBlock();
		for (int32 I = 0; I < Keys.Num(); I++)
		{
			Constructor.AddLinef(TEXT("%s.Add(%s), %s);"), *MapName, *LexToCpp(Keys[I]), *Values[ValuesOffset + I]);
		}
		Constructor.EndBlock();
	}

	template<typename T>
	static FString ClassString()
	{
		return FString::Printf(TEXT("%s%s"), T::StaticClass()->GetPrefixCPP(), *T::StaticClass()->GetName());
	}

	template<typename T>
	static FString SoftObjectPtrString()
	{
		return FString::Printf(TEXT("TSoftObjectPtr<%s>"), *ClassString<T>());
	}
	template<typename T>
	static FString SoftClassPtrString()
	{
		return FString::Printf(TEXT("TSoftClassPtr<%s>"), *ClassString<T>());
	}

	template<typename T>
	static FString ObjectDefaultString(T* Object)
	{
		if (!Object)
		{
			return "";
		}
		return FString::Printf(TEXT("%s(FSoftObjectPath(\"%s\"))"), *SoftObjectPtrString<T>(), *Object->GetPathName());
	}
	template<typename T>
	static FString ClassDefaultString(UClass* Class)
	{
		if (!Class)
		{
			return "";
		}
		return FString::Printf(TEXT("%s(FSoftObjectPath(\"%s\"))"), *SoftClassPtrString<T>(), *Class->GetPathName());
	}

	template<typename T>
	static FString PickerDefaultString(const T& Picker)
	{
		const FString Struct = T::StaticStruct()->GetStructCPPName();
		if (Picker.IsClass())
		{
			return Struct + "(" + ClassDefaultString<typename T::UGenerator>(Picker.Class) + ")";
		}
		else
		{
			return Struct + "(" + ObjectDefaultString(Picker.Object) + ")";
		}
	}

	static FString LoadObjectString(const FString& Name)
	{
		return Name + ".LoadSynchronous()";
	}
};
