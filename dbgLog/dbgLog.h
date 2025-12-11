// Copyright ©  Jack Holland.
//
// DbgLOG is free software. It comes without any warranty, to the extent permitted
// by applicable law. You can redistribute it and/or modify it under the terms
// of the Do What The Fuck You Want To Public License, Version 2, as published
// by Sam Hocevar:
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                    Version 2, December 2004
//
// Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
//
// Everyone is permitted to copy and distribute verbatim or modified
// copies of this license document, and changing it is allowed as long
// as the name is changed.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//
//  0. You just DO WHAT THE FUCK YOU WANT TO.
#pragma once
#include "Private/LLog.h"
#include "VisualLogger/VisualLogger.h"
#include "Misc/EngineVersionComparison.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "dbgLog.generated.h"

#define KEEP_DBG_LOG (!UE_BUILD_SHIPPING) || (USE_LOGGING_IN_SHIPPING) 

// #define DBG_API MYMODULE_API // If you would rather define the modules API here you can, but otherwise stick to using the build.cs definition.



/**
 *	This library is built on top of LLog - https://github.com/landelare/llog and adds some QOL features as well as implementing
 *	the actual macros and Builder pattern for logging (more on this later).
 *	But a HUGE shout out goes to Laura for her work and logging articles https://landelare.github.io/2022/04/28/better-ue_log.html
 *
 *
 *
 *	Steps for installation:
 * - 1) Download the "dbgLog" folder (either by cloning the repo or downloading the latest release).
 * - 2) Place folder inside of your module where you wish for the logging macro to exist, typically your main game core module,
 *	but it also could be some other module like a custom debugging specific module.
 * - 3) Modify the `Build.cs` file of module holding the folder to define the export API for it, just copy paste this but replace MYMODULE_API with yours, this only needs to be done for the module that holds the folder.
 *		- `PublicDefinitions.Add("DBG_API=MYMODULE_API");` 
 * - 4) Ensure any module that uses this macro has "Slate" defined as a dependency, if it doesn't just paste this snippet in that modules `Build.cs`
 *		-  `PrivateDependencyModuleNames.Add("Slate");`
 * - 5) That's it, now just include "dbgLog/dbgLog.h" anywhere you wish to use the macro from. You can reorganize the file/folder structure however you see fit but just ensure to update the includes within them.
 *
 *
 *	This library is built off of the standard libraries `std::format` and has better formatting support than unreals standard
 *	FString::Format (which doesn't even let you specify arg placements or decimal places) and is a LOT
 *	safer than Printf formats which crash at runtime by simply passing the wrong % specifier.
 *
 *	std::format will compile-time assert if anything is not correct or would cause a crash.
 *	
 *	Please See https://en.cppreference.com/w/cpp/utility/format/spec for a more in-depth look at formatting options
 *	which are supported here, this allows you define floating point precision, spacing etc.
 *
 *	
 *	There are really only two macros to concern yourself with -
 *	- `dbgLOG`
 *	- `dbgLOGV`.
 *
 *
 *	Side note: You can easily redefine these macros to fit your own projects naming scheme by simply copy pasting this and renaming it to your own preference:
 *	#define MYOWNLOG(Msg, ...) dbgLOG(Msg, __VA_ARGS__)
 *  #define MYOWNLOGV(Args, Msg, ...) dbgLOGV(Args, Msg, __VA_ARGS__)
 *
 *  There are also some built in console commands such as:
 *  "dbgLog.DisableCategory [SomeCategories]" where SomeCategories are the ones you want to disable.
 *  "dbgLog.EnableCategory [SomeCategories]" where SomeCategories are the ones you want to re-enable.
 *  "dbgLog.PrintCategoriesStates" Can be used to print the current state of the categories registered with us.
 *
 *  These are built ON TOP of the existing verbosity system in unreal and are just a nice thing to have for quick enabling/disabling
 *  without modifying a logs verbosity directly. Now onto explaining the rest.
 *
 *
 *	dbgLOG is your standard log macro that takes a format message and optional logs, some example usages are as follows:
 *	
 *	- dbgLOG("Regular log");														// Outputs: "Regular log"
 *	
 *	- dbgLOG("Mr Tim {0}", "Sweeney");												// Outputs: "Mr Tim Sweeney"
 *	
 *	- dbgLOG("{1}{0}", "Bar", "Foo");												// Outputs: "Foo Bar" (see how we can reorder the arguments depending on their placement)
 *	
 *	- dbgLOG("{1}{0} {2}", "Bar", "Foo", 42);										// Outputs: "Foo Bar 42"
 *	
 *	- dbgLOG("{0:.3f}", 3.14159265);												// Outputs: "3.142" (see how we specify :.3f to output only up to three decimal places)
 *	
 *	- dbgLOG("{0}", 3.141592653589793238);											// Outputs: "3.141592653589793238" (outputs the entire value)
 *	
 *
 *	We can also log items such as; TArray, TMap, Tuples, C style arrays, enums, structs, pointers etc.
 *	Containers are recursive and call the same format function for each of their elements within.
 *
 *	You can also add support for your own logging formats based on the type by extending the `if constexpr` chain found inside of `LLog.h` with the `FormatArgument` function,
 *	this is more hands on and usually isn't required.
 *
 *
 *	Some examples of logging built in types are as follows
 *
 *	
 *	// Outputs: "ESomeEnum::Val_3", if the enum is not using reflection it would just output its value as an int
 *	ESomeEnum SomeEnum = ESomeEnum::Val_3;
 *	dbgLOG("{0}", SomeEnum);
 *
 *
 * 	// Outputs: "[0, 1, 2, 3, 4]"
 * 	int MyStaticArray[5] = {0, 1, 2, 3, 4};
 * 	dbgLOG("{0}", MyStaticArray);
 *
 * 
 * 
 * 	// Outputs: "[5, 6, 7, 8, 9]"
 * 	TArray<int> MyDynamicArray = {5, 6, 7, 8, 9};
 * 	dbgLOG("{0}", MyDynamicArray);
 *
 *
 *
 *	USTRUCT(BlueprintType)
 *	struct FMyReflectedStruct
 *	{
 *		GENERATED_BODY()
 *	public:
 *		UPROPERTY(EditAnywhere)
 *		FString SomeStr = TEXT("Foo");
 *	
 *		UPROPERTY(EditAnywhere)
 *		int SomeInt = 42;
 *		
 *		UPROPERTY(EditAnywhere)
 *		float SomeFloat = 3.14f;
 *	};
 *
 *	// This is able to be logged because we make use of Unreal's reflection and log it via ExportText which gives us
 *	// the name and value for each exposed property.
 *	// Outputs: "(SomeStr="Foo",SomeInt=42,SomeFloat=3.14)"
 *	FMyReflectedStruct S{};
 *	dbgLOG("{0}", S);
 *
 *
 *
 *	struct FMyRegularStruct
 *	{
 *		FString ToString() const {return FString::Format(Text("{0} {1}"), {SomeStr, MyValue}); }
 *		FString SomeStr = TEXT("Foo");
 *		int MyValue = 42;
 *	};
 *	
 *	// This is able to be logged without reflection needed because the type has a public `ToString` member function which we call, if we didn't have that or a "GetName()"
 *	// then we would not be able to log the type by default and you would need to log it like normal by getting each member that you want out IE; "dbgLOG("MyVal {0}", S.MyValue)"
 *	// Outputs: "Foo 42"
 *	FMyRegularStruct S{};
 *	dbgLOG("{0}", S);
 *
 *
 *
 *
 *  // We can log pointers like so, no need to check for null (that also goes for non uobject pointers):
 * 	// Outputs: "UObject Pointer: (UMyObject*)Name"
 * 	UMyObject* ThisPointer = this;
 * 	dbgLOG("UObject Pointer: {0}", this);
 * 
 * 	// Outputs: "UObject Pointer: (MyType*)Null Ptr" (Doesn't crash)
 * 	ThisPointer = nullptr;
 * 	dbgLOG("UObject Pointer: {0}", ThisPointer);
 * 
 *
 * 	struct FHealth
 *	{
 *		FString ToString() const {return FString::Format(TEXT("Health: {0}"), {Health});}
 *		float Health = 95.f;
 *	};
 *	
 * 	TMap<int, FHealth> SomeStructMap = {
 * 		{0, {95.f}},
 * 		{1, {85.f}},
 * 		{2, {75.f}},
 * 		{3, {65.f}},
 * 		{4, {55.f}}
 * 	};
 *
 * 	// Outputs: "[ {0, Health: 95}, {1, Health: 85}, {2, Health: 75}, {3, Health: 65}, {4, Health: 55} ]"
 * 	dbgLOG("{0}", SomeStructMap);
 * 
 *
 *
 *
 * 
 *	---------------------------------------- Verbose log -----------------------------------------
 * 
 *	`dbgLOGV` is just an extension of `dbgLOG` with the added possibility for builder args, if you are not familiar with this concept it basically
 *	just allows you to chain arguments together, for example ` .ThingOne(Value).ThingTwo(Value).ThingThree(Value)` and it works because each chained
 *	argument just returns a reference to the original builder object.
 *
 *	This style of a log macro allows for basically multiple variadic args (the builder and the format args).
 *
 *	Here are some examples of the macro in use:
 *
 *	// Sets this logs verbosity to Warning
 *	dbgLOGV(.Warn(), "Warning log");
 *	
 *	// Sets this logs verbosity to Error
 *	dbgLOGV(.Error(), "Error log");
 *
 *
 *
 *	// Here we choose to make this log output to both screen and console (the options are `Console, Screen, ConsoleAndScreen`)
 *	// also note how we chain multiple args. The time we show the message on screen for is defined from the logs verbosity - in this case it is Error so 30s
 *	dbgLOGV(.Error().ScreenAndConsole(), "{0}", 3.14);
 *
 *
 *	// WCO (short for World Context Object) gives us more contextual information for output of the log
 *	// in the format of `[Client | Instance: 0]: My value is - whatever`, where Client could be "Dedicated Server", "Standalone", etc.
 *	
 *	// Also we can runtime log with *whatever* category name we want (the only thing of note is we prefix all categories with "dbg" to ensure
 *	// we have no runtime clashes with existing categories set by the engine)
 *	dbgLOGV(.Category("MyCustomCategory").WCO(this), "My value is - {0}", GetSomeValue);
 *
 *	// This is the same as above but this one takes an existing log category that you or the engine has predefined
 *	// so no need to worry about losing your favorite predefined log categories.
 *	dbgLOGV(.Category(LogTemp).WCO(this), "My value is - {0}", GetSomeValue());
 *
 *
 *	// Condition is great for ensuring conditions and branches for logging are not actually kept in shipping builds (this is my favorite)
 *	// One use case is obviously getter/predicate functions like HasAmmo() but you can also do things like ".Condition(MyCvar.GetValueOnGameThread())" 
 *	dbgLOGV(.Condition(false), "I only log if the condition is true");
 *
 *
 *		
 *	// Spawns a slate notification popup in the bottom right - the verbosity of the log dictates the notify show duration.
 *	dbgLOGV(.LogToSlateNotify(), "My Slate Notify"); // Shows for 6s
 *	dbgLOGV(.LogToSlateNotify().Error(), "My Slate Notify"); // Shows for 30s
 *
 *	
 *	// Spawns a message box dialog for the programmer to interact with, also provides a callback for what they chose - you can pass null if you
 *	// do not want to respond to the selected option.
 *	dbgLOGV(.LogToMessageDialog( [](EAppReturnType::Type Result)
 *	{
 *		dbgLOG( "The user selected {0}", Result);
 *	}, EAppMsgType::YesNo ), "Should Do Thing?" );
 *
 *
 *  Visual logger and Debug shapes are also supported as Builder args, examples below:
 *  
 *  // Draws this actor's bounds into the visual logger (the reason for this, this is because
 *  // it takes the log owner and the actor to draw which can be different in certain circumstances)
 *  // We can only have one type of visual log per macro though.
 *	dbgLOGV(.VisualLogBounds(this, this), "Visual Log Test {0}", GetWorld()->GetTimeSeconds());
 *
 *	
 *	// Many different DrawDebugShape functions are supported to ensure they are
 *	// compiled out of shipping builds and the usage of this library is streamlined
 *	dbgLOGV(.DrawDebugSphere(this, GetActorLocation(), 50.f, 12), "Drawing Sphere...");
 */






UENUM(BlueprintType)
enum EDbgLogOutput : uint8
{
	// Outputs only to the console
	Con = 0 UMETA(DisplayName = "Console"),

	// Outputs only the the screen (if possible, otherwise falls back to the console)
	Scr UMETA(DisplayName = "Screen"),
	
	// Outputs to both the screen (if possible) and console.
	Both UMETA(DisplayName = "Screen and Console")
};




#if KEEP_DBG_LOG


// This macro was added in 5.6 so in order to not clash on that version or greater I am prefixing with bf_
#define bf_UE_VERSION_NEWER_THAN_OR_EQUAL(MajorVersion, MinorVersion, PatchVersion)\
	UE_GREATER_SORT(ENGINE_MAJOR_VERSION, MajorVersion, UE_GREATER_SORT(ENGINE_MINOR_VERSION, MinorVersion, UE_GREATER_SORT(ENGINE_PATCH_VERSION, PatchVersion, true)))


#ifndef DBG_API
#error "If you are seeing this error, please add the following line to the Build.cs file of the module which houses log folder - `PublicDefinitions.Add("DBG_API=MYMODULE_API");` (replacing MYMODULE_API with your own), also ensure the module has \"Slate\" as a dependency."
#endif


DBG_API DECLARE_LOG_CATEGORY_EXTERN(dbgLOG, Display, All);


// Should not be used directly. 
#define _INTERNAL_DBGLOGV(Args, Msg, Name, ...) do\
{\
    DBG::Log::DbgLogArgs Name{}; \
    Name Args;\
    DBG::Log::Log(__COUNTER__, std::source_location::current(), Name,  TEXT(Msg) __VA_OPT__(,) __VA_ARGS__); \
} while(false)


/**
 *	This your standard formatted log message, could either be `"Hello World" or "Hello World {0}" where 0 is placement
 * for your log arguments, for example:
 * \code
 * // This takes no args.
 * dbgLOG("Hello World");
 * \endcode
 * \code
 * // This takes any amount of args.
 * dbgLOG("Hello World {0}", 42);
 * \endcode
 * \code
 * // This is how you would format a float/double to only show 3 places - similar to printf's `%.3f`
 * dbgLOG("Hello World {0:.3f}", 3.1415926535);
 * \endcode
 */
#define dbgLOG(Msg, ...) do\
{\
    DBG::Log::Log(__COUNTER__, std::source_location::current(), DBG::Log::DbgLogArgs{}, TEXT(Msg) __VA_OPT__(,) __VA_ARGS__);\
}while(false)


/**
 * This a slightly more advanced log where you have the ability to set extra information about the log, such as its verbosity, where it outputs (screen or console) and a lot more.
 * This log is more less the same as the standard log but with one param at the beginning that begins with the `.` operator to access the log args object.
 * For example:
 *
 * \code
 * // This outputs our log to only the screen but Screen could have also been "Console" or "ScreenAndConsole"
 * 
 * dbgLOGV(.Screen(), "Hello World"); 
 * \endcode
 * \code
 * // This outputs as long as the condition is true, the input could be some function like
 * // `HasAmmo()` and it only logs if we have ammo.
 * 
 * dbgLOGV(.Condition(true), "Hello World");
 * \endcode
 * \code
 * // Lastly you can chain these args like so.
 * // This prints "Hello World 42" along with the source location of the macro
 * // (file, line number and function it was called from),
 * // to the console with a yellow warning verbosity and has the runtime category "dbgPlayer"
 * 
 * dbgLOGV(.LogSourceLoc().ScreenAndConsole().Warn().Category("Player"), "Hello World {0}", 42);
 * \endcode
 */
#define dbgLOGV(Args, Msg, ...) _INTERNAL_DBGLOGV(Args, Msg, _CONCAT(LogArgs, __COUNTER__) __VA_OPT__(,) __VA_ARGS__)





namespace DBG::Log
{
	struct DbgLogArgs
	{
		using ThisClass = DbgLogArgs;
		template<typename... A>
		friend void Log(int32 UniqueIdentifier, std::source_location Location, 
				DbgLogArgs LogArgs, 
				std::wformat_string<TFormatted<A>...> Format, 
				A&&... Args);
		
		DbgLogArgs() = default;

		// The log category, can be anything you want but `dbg` is appended due to possible naming conflicts with existing categories (which can cause an asset).
		ThisClass& Category(FName CategoryName) {LogCategoryName = CategoryName; return *this;}
		
		// Takes a log category to use when logging.
		ThisClass& Category(const FLogCategoryBase& InCategory) {LogCategory = &InCategory; return *this;}
		
		// Verbosity is the level of the log message, usage is `.Verbosity(ELogVerbosity::Warning)` etc.
		ThisClass& Verbosity(ELogVerbosity::Type Verb) {VerbosityValue = Verb; return *this;}

		// Defaults the log verbosity to type: Verbose
		ThisClass& Verbose() {VerbosityValue = ELogVerbosity::Verbose; return *this;}

		// Defaults the log verbosity to type: Warning
		ThisClass& Warn() {VerbosityValue = ELogVerbosity::Warning; return *this;}

		// Defaults the log verbosity to type: Error
		ThisClass& Error() {VerbosityValue = ELogVerbosity::Error; return *this;}
		
		// Defaults the log verbosity to type: Fatal (This will crash the application, use with caution)
		ThisClass& Fatal() {VerbosityValue = ELogVerbosity::Fatal; return *this;}

		// World context Object, can be used to append to the log the PIE ID as well as net mode.
		ThisClass& WCO(const UObject* CO) {WCOResultValue = GEngine->GetWorldFromContextObject(CO, EGetWorldErrorMode::ReturnNull); return *this;}

		// Outputs this log to the screen if possible (otherwise falls back to the console)
		ThisClass& Screen() {OutputDestinationValue = EDbgLogOutput::Scr; return *this;}

		// Outputs this log to the console.
		ThisClass& Console() {OutputDestinationValue = EDbgLogOutput::Con; return *this;}
		
		// Outputs this log to the console and screen if possible (otherwise just outputs to the console)
		ThisClass& ScreenAndConsole() {OutputDestinationValue = EDbgLogOutput::Both; return *this;}
		
		// Appended to the log message as `[PREFIX]: Regular Log Msg`.
		ThisClass& Prefix(const FString& PrefixStr) {PrefixValue = PrefixStr; return *this;}
		
		// Color for screen logs.
		ThisClass& ScrnColor(FColor Col) {ScreenColorValue = Col; return *this;}
		
		// Duration for screen logs.
		ThisClass& ScrnDuration(float Duration) {ScreenDurationValue = Duration; return *this;}
		
		// Optional key for screen logs.
		ThisClass& ScrnKey(int32 Key) {ScreenKeyValue.Emplace(Key) ; return *this;}
		
		// Condition for the log, great for things like CVars or only logging under certain conditions.
		ThisClass& Condition(bool Condition) {bLogConditionValue = Condition; return *this;}

		// If called, it enables logging of the source location, things like file name, line number etc.
		ThisClass& LogSourceLoc() {bLogSourceLocation = true; return *this;}

		// If called, enables the default format for the date and time which is prepended to the log.
		ThisClass& LogDateAndTime() {bLogDateAndTime = true; return *this;}

		/**
		 * Lets this macro log to a slate notification popup in the bottom corner
		 * @param bOnlyUseThisLog If true we do not attempt to also log to the console or screen.
		 */
		ThisClass& LogToSlateNotify(bool bOnlyUseThisLog = false)
		{
			bOnlyLogToSlateNotify = bOnlyUseThisLog;
			bLogToSlateNotify = true;
			return *this;
		}

		/**
		 * Logs the provided message to a dialog box the user can interact with.
		 * @param Response User response to the dialog
		 * @param MsgType What type of dialog should be shown
		 * @param bOnlyUseThisLog If true we do not attempt to also log to the console or screen.
		 */
		ThisClass& LogToMessageDialog(
			const TFunction<void(EAppReturnType::Type UserResponse)>& Response,
			EAppMsgType::Type MsgType, bool bOnlyUseThisLog = false)
		{
			AppMessageResponse = Response;
			AppMsgType = MsgType;
			bOnlyLogToMessageDialog = bOnlyUseThisLog;
			bLogToMessageDialog = true;
			return *this;
		}
		
		/** 
		 * Logs a message to the editor message log (this is a window that pops up showing you any errors or warnings)
		 * You would see this when accessing null in a BP graph for example after you quit PIE.
		 * @param bShouldShowEditorMessageLogImmediately If true we instantly spawn the editor message log window,
		 * otherwise you need to manually check the window
		 */
		ThisClass& LogToEditorMessageLog(
			bool bShouldShowEditorMessageLogImmediately = false)
		{
			bLogToEditorMessageLog = true;
			bShowEditorMessageLogImmediately = bShouldShowEditorMessageLogImmediately;
			return *this;
		}
		
		/**
		 * Logs the date and time with the provided format
		 * %y = Year YY, %Y = Year YYYY, %m = Month 01-12, %d = Day 01-31
		 * %h = Hour 0-12, %H = Hour 00-23, %M = Minute 00-59, %S = Second 00-59, %s = Millisecond 000-999
		 * Example `.LogDateAndTime(TEXT("%d/%m/%y %H:%M:%S"))` outputs `03/04/2025 06:42:29`
		 */
		ThisClass& LogDateAndTime(TStringView<TCHAR> Format) {bLogDateAndTime = true; DateTimeFormat = Format; return *this;}


		ThisClass& DrawDebugCapsule(
			const UObject* WorldContextObject, const FVector& Center,
			float HalfHeight, float Radius,
			const FQuat& Rotation, const FColor& Color,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);

		
		ThisClass& DrawDebugCone(
			const UObject* WorldContextObject, const FVector& Origin,
			const FVector& Direction, float Length,
			float AngleWidth, float AngleHeight,
			int32 NumSides, const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);

		
		ThisClass& DrawDebugCylinder(
			const UObject* WorldContextObject, const FVector& Start,
			const FVector& End, float Radius,
			int32 Segments, const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);

		
		ThisClass& DrawDebugDirectionalArrow(
			const UObject* WorldContextObject, const FVector& LineStart,
			const FVector& LineEnd, float ArrowSize,
			const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);

		
		ThisClass& DrawDebugLine(
			const UObject* WorldContextObject, const FVector& LineStart,
			const FVector& LineEnd, const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);

		
		ThisClass& DrawDebugPoint(const UObject* WorldContextObject,
			const FVector& Position, float Size,
			const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0);

		
		ThisClass& DrawDebugSphere(
			const UObject* WorldContextObject, const FVector& Center,
			float Radius, int32 Segments,
			const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);

		
		ThisClass& DrawDebugString(
			const UObject* WorldContextObject, const FVector& TextLocation,
			const FString& Text, AActor* TestBaseActor = nullptr,
			const FColor& TextColor = FColor::Orange,
			float Duration = -1.f);

		
		ThisClass& DrawDebugBox(
			const UObject* WorldContextObject, const FVector& Center,
			const FVector& Extent, const FColor& Color = FColor::Orange,
			bool bPersistentLines = false, float LifeTime = -1.f,
			uint8 DepthPriority = 0, float Thickness = 0);


		// you may only use one visual log per macro.

		// Logs text with the visual logger system.
		DbgLogArgs& VisualLogText(
			const UObject* Owner,
			bool bOnlyLogVisually = true);

		
		// Logs a sphere with the visual logger system.
		DbgLogArgs& VisualLogSphere(
			const UObject* Owner, const FVector& Location,
			float Radius,
			FColor SphereColor = FColor::Orange,
			bool bDrawWireframe = false,
			bool bOnlyLogVisually = true);

		
		// Logs a box with the visual logger system.
		DbgLogArgs& VisualLogBox(
			const UObject* Owner, const FVector& MinExtent,
			const FVector& MaxExtent, const FVector& Location,
			const FRotator& Rotation = FRotator::ZeroRotator,
			FColor BoxColor = FColor::Orange,
			bool bDrawWireframe = false,
			bool bOnlyLogVisually = true);
		
		
		// Logs a cone with the visual logger system.
		DbgLogArgs& VisualLogCone(
			const UObject* Owner, const FVector& Location,
			const FVector& Direction, float Length,
			float Angle, FColor ConeColor = FColor::Orange,
			bool bDrawWireframe = false, bool bOnlyLogVisually = true);
		

		// Logs a line with the visual logger system.
		DbgLogArgs& VisualLogLine(
			const UObject* Owner, const FVector& Start,
			const FVector& End, float Thickness = 1.f,
			FColor LineColor = FColor::Orange,
			bool bOnlyLogVisually = true);

		
		// Logs an arrow with the visual logger system.
		DbgLogArgs& VisualLogArrow(
			const UObject* Owner, const FVector& Start,
			const FVector& End, FColor ArrowColor = FColor::Orange,
			bool bOnlyLogVisually = true);

		
		// Logs a disk with the visual logger system.
		DbgLogArgs& VisualLogDisk(
			const UObject* Owner, const FVector& Start,
			const FVector& UpDir, float Radius,
			FColor DiskColor = FColor::Orange,
			uint16 Thickness = 1, bool bOnlyLogVisually = true,
			bool bDrawWireframe = true);

		
		// Logs a capsule with the visual logger system.
		DbgLogArgs& VisualLogCapsule(
			const UObject* Owner, const FVector& Base,
			const FRotator& Rotation, float Radius,
			float HalfHeight, FColor CapsuleColor = FColor::Orange,
			bool bDrawWireframe = false, bool bOnlyLogVisually = true);

				
		// Logs the provided actors bounds into the visual logger system.
		DbgLogArgs& VisualLogBounds(
			const UObject* Owner, const AActor* ActorToGetBoundsFrom,
			FColor BoundsColor = FColor::Orange, bool bDrawWireframe = false,
			bool bOnlyLogVisually = true);

	private:
		enum struct EDbgVisualLogShape : uint8
		{
			None,
			Sphere,
			Box,
			Cone,
			Line,
			Arrow,
			Disk,
			Capsule,
		};

		const FLogCategoryBase* LogCategory			= nullptr;
		const UWorld* WCOResultValue				= nullptr;
		const UObject* VisualLoggerOwnerValue		= nullptr;
		TStringView<TCHAR> DateTimeFormat			= nullptr;
		TFunction<void(EAppReturnType::Type Response)> AppMessageResponse = nullptr;
		
		FString PrefixValue							= {};
		FName LogCategoryName						= {};
		FColor ScreenColorValue						= FColor::Transparent;
		
		float ScreenDurationValue					= -1.f;
		TOptional<int32> ScreenKeyValue				= NullOpt;

		EDbgLogOutput OutputDestinationValue		= EDbgLogOutput::Con;
		

		// All the visual logger related variables.
		EDbgVisualLogShape VisualLogShapeValue		= EDbgVisualLogShape::None;
		FVector VisualLogShapeLocationValue			= FVector::ZeroVector;
		FRotator VisualLogShapeRotationValue		= FRotator::ZeroRotator;
		FVector VisualLogShapeScaleValue			= FVector::OneVector;

		// Reuse these for context dependent shapes.
		// For example a sphere just uses VectorOne.X for radius but a
		// Box needs a min and max extent.
		FVector VisualLogVectorOne 					= FVector::ZeroVector;
		FVector VisualLogVectorTwo 					= FVector::ZeroVector;

		FColor VisualLogShapeColorValue				= FColor::Orange;
		
		uint16 bLogConditionValue:1 				= true;
		uint16 bLogSourceLocation:1 				= false;
		uint16 bLogToSlateNotify:1 					= false;
		uint16 bOnlyLogToSlateNotify:1 				= false;
		uint16 bLogToMessageDialog:1 				= false;
		uint16 bOnlyLogToMessageDialog:1 			= false;
		uint16 bLogToEditorMessageLog:1 			= false;
		uint16 bShowEditorMessageLogImmediately:1 	= false;
		uint16 bLogDateAndTime:1 					= false;
		uint16 bDrawWireframeValue:1 				= false;
		uint16 bOnlyUseVisualLogger:1 				= false;

		EAppMsgType::Type AppMsgType				= EAppMsgType::Type::Ok;
		ELogVerbosity::Type VerbosityValue			= ELogVerbosity::Display;
	};

	struct FDbgLogSingleton
	{
		FDbgLogSingleton();

		template<bool bAddIfMissing>
		bool IsCategoryDisabled(FName CategoryName)
		{
			if (FRegisteredCategory* Cat = CategoryMap.Find(CategoryName))
			{
				return Cat->bState == false;
			}
			
			if constexpr (bAddIfMissing)
			{
				CategoryMap.Emplace(CategoryName, {true});
			}
			return false;
		}

		template<bool bAddIfMissing>
		void SetCategoryState(FName CategoryName, bool bNewState)
		{
			if (FRegisteredCategory* Cat = CategoryMap.Find(CategoryName))
			{
				Cat->bState = bNewState;
				return;
			}

			if constexpr (bAddIfMissing)
			{
				UE_LOG(dbgLOG, Warning,
					TEXT("Failed to to locate category %s, making state entry anyway."),
					*CategoryName.ToString());
				
				CategoryMap.Emplace(CategoryName, {bNewState});
			}
		}

		struct FRegisteredCategory
		{
			bool bState;
		};
		TMap<FName, FRegisteredCategory> CategoryMap;
	};

	DBG_API extern FDbgLogSingleton GDbgLogSingleton;
	
	template<typename... A>
	void Log(int32 UniqueIdentifier, std::source_location Location,
		DbgLogArgs LogArgs, std::wformat_string<TFormatted<A>...> Format,
		A&&... Args)
	{
		if(LogArgs.VerbosityValue == ELogVerbosity::NoLogging
		|| LogArgs.bLogConditionValue == false)
		{
			return;
		}
		
		// Work out which category to use, we we're either passed that exists, passed one we need to create ourself, or use the default one.
		const FLogCategoryBase* LogCategory = nullptr;
		TOptional<FLogCategory<ELogVerbosity::Display, ELogVerbosity::All>> OptionallyCreatedCategory;
		
		if (LogArgs.LogCategory != nullptr)
		{
			LogCategory = LogArgs.LogCategory;
		}
		else if (LogArgs.LogCategoryName.IsNone() == false && LogArgs.LogCategoryName != dbgLOG.GetCategoryName())
		{
			FName CategoryName = FName{FString::Format(
				TEXT("dbg{0}"), {LogArgs.LogCategoryName.ToString()})};
			
			OptionallyCreatedCategory.Emplace(CategoryName);
			LogCategory = &OptionallyCreatedCategory.GetValue();
		}
		else // Lastly, fallback to the default if the user supplied no category.
		{
			LogCategory = &dbgLOG;
		}
		
		// User has disabled it via the `dbgLog.DisableCategory Foo`, must re-enable it via `dbgLog.EnableCategory Foo`
		if (GDbgLogSingleton.IsCategoryDisabled<true>(LogCategory->GetCategoryName()))
		{
			return;
		}
		
		// Format the actual log provided from the user.
		FString Message = FormatMessage(std::move(Format), std::forward<A>(Args)...);


		// Configure how we present the log now.
		
		static auto NetModeToStr = [](ENetMode Mode) -> FString
		{
			static FString StandAlone		{TEXT("Standalone")};
			static FString DedicatedServer	{TEXT("Dedicated Server")};
			static FString ListenServer		{TEXT("Listen Server")};
			static FString Client			{TEXT("Client")};
			static FString Max				{TEXT("MAX")};
			static FString Unknown			{TEXT("Unknown")};
			
			switch (Mode)
			{
				case NM_Standalone:			return StandAlone;
				case NM_DedicatedServer:	return DedicatedServer;
				case NM_ListenServer:		return ListenServer;
				case NM_Client:				return Client;
				case NM_MAX:				return Max;
				default:					return Unknown;
			}
		};
		 
		static auto WorldToString = [](const UWorld* W)
		{
			FWorldContext* WC = GEngine->GetWorldContextFromWorld(W);
			if (!WC)
			{
				static FString NullWCO = TEXT("NullWorld");
				return NullWCO;
			}
			
			return FString::Format(TEXT("{0} | Instance: {1}"),
				{NetModeToStr(W->GetNetMode()), WC->PIEInstance});
		};

		
		// Make the source location a little nicer to read.
		static auto SourceLocationToStr = [](std::source_location& Loc) -> FString
		{
			FString FuncName(Loc.function_name());
			FuncName.ReplaceInline(TEXT(" __cdecl"), TEXT(""));
			
			return FString::Format(TEXT("[File: {0} ({1}), {2}]"),
				{
					FPaths::GetCleanFilename(StringCast<wchar_t>(Loc.file_name()).Get()),
						Loc.line(), FuncName
				}
			);
		};


		TStringBuilder<96> MessagePrefixBuilder;

		if (LogArgs.bLogDateAndTime)
		{
			if (LogArgs.DateTimeFormat != nullptr)
			{
				MessagePrefixBuilder.Appendf(TEXT("(%s) "),
					*FDateTime::Now().ToString(LogArgs.DateTimeFormat.GetData()));
			}
			else
			{
				MessagePrefixBuilder.Appendf(TEXT("(%s) "),
					*FDateTime::Now().ToString());
			}
		}

		if (LogArgs.PrefixValue.Len() > 0)
		{
			MessagePrefixBuilder.Appendf(TEXT("[%s] "),
				*LogArgs.PrefixValue);
		}

		if (const UWorld* W = LogArgs.WCOResultValue)
		{
			if (LogArgs.bLogSourceLocation)
			{
				MessagePrefixBuilder.Appendf(TEXT("[%s] %s "),
					*WorldToString(W), *SourceLocationToStr(Location));
			}
			else
			{
				MessagePrefixBuilder.Appendf(TEXT("[%s] "),
					*WorldToString(W));
			}
		}
		else if (LogArgs.bLogSourceLocation)
		{
			MessagePrefixBuilder.Appendf(TEXT("%s "),
				*SourceLocationToStr(Location));
		}

		if (MessagePrefixBuilder.Len() > 0)
		{
			Message.InsertAt(0, MessagePrefixBuilder.ToString());
		}
		
		
#if ENABLE_VISUAL_LOG
		// The reason for manually calling these instead of using VLOG is that VLOG wanted to be annoying and
		// assume our log verbosity is a constant IE `ELogVerbosity::MacroVerbosity`
		if(LogArgs.VisualLoggerOwnerValue && FVisualLogger::IsRecording())
		{
			switch (LogArgs.VisualLogShapeValue)
			{
				case DbgLogArgs::EDbgVisualLogShape::None:
					{
						FVisualLogger::CategorizedLogf(
							LogArgs.VisualLoggerOwnerValue, *LogCategory,
							LogArgs.VerbosityValue, TEXT("%s"), *Message);
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Sphere:
					{
	#if UE_VERSION_NEWER_THAN(5, 4, 0)
						FVisualLogger::SphereLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne.X,
							LogArgs.VisualLogShapeColorValue,
							LogArgs.bDrawWireframeValue,
							TEXT("%s"), *Message);
	#else
						FVisualLogger::GeometryShapeLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne.X,
							LogArgs.VisualLogShapeColorValue,
							TEXT("%s"), *Message);
	#endif
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Box:
					{
	#if UE_VERSION_NEWER_THAN(5, 4, 0)
						FVisualLogger::BoxLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							FBox{LogArgs.VisualLogVectorOne, LogArgs.VisualLogVectorTwo},
							FMatrix{FScaleMatrix(LogArgs.VisualLogShapeScaleValue) *
									FRotationMatrix(LogArgs.VisualLogShapeRotationValue) *
									FTranslationMatrix(LogArgs.VisualLogShapeLocationValue)},
							LogArgs.VisualLogShapeColorValue,
							LogArgs.bDrawWireframeValue,
							TEXT("%s"), *Message);
	#else
						FVisualLogger::GeometryBoxLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							FBox{LogArgs.VisualLogVectorOne, LogArgs.VisualLogVectorTwo},
							FMatrix{FScaleMatrix(LogArgs.VisualLogShapeScaleValue) *
									FRotationMatrix(LogArgs.VisualLogShapeRotationValue) *
									FTranslationMatrix(LogArgs.VisualLogShapeLocationValue)},
							LogArgs.VisualLogShapeColorValue,
							TEXT("%s"), *Message);
	#endif
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Cone:
					{
	#if UE_VERSION_NEWER_THAN(5, 4, 0)
						FVisualLogger::ConeLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogVectorTwo.X,
							LogArgs.VisualLogVectorTwo.Y,
							LogArgs.VisualLogShapeColorValue,
							LogArgs.bDrawWireframeValue,
							TEXT("%s"), *Message);
	#else
						FVisualLogger::GeometryShapeLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogVectorTwo.X,
							LogArgs.VisualLogVectorTwo.Y,
							LogArgs.VisualLogShapeColorValue,
							TEXT("%s"), *Message);
						
	#endif
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Line:
					{
	#if UE_VERSION_NEWER_THAN(5, 4, 0)
						FVisualLogger::SegmentLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogShapeColorValue,
							static_cast<uint16>(LogArgs.VisualLogVectorTwo.X),
							TEXT("%s"), *Message);
	#else
						FVisualLogger::GeometryShapeLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogShapeColorValue,
							static_cast<uint16>(LogArgs.VisualLogVectorTwo.X),
							TEXT("%s"), *Message);
	#endif
	
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Arrow:
					{
	#if bf_UE_VERSION_NEWER_THAN_OR_EQUAL(5, 6, 0)
						FVisualLogger::ArrowLineLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogShapeColorValue, 0,
							TEXT("%s"), *Message);
	#else
						FVisualLogger::ArrowLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogShapeColorValue,
							TEXT("%s"), *Message);
	#endif
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Disk:
					{
	#if bf_UE_VERSION_NEWER_THAN_OR_EQUAL(5, 6, 0)
						FVisualLogger::DiscLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogVectorTwo.X,
							LogArgs.VisualLogShapeColorValue,
							static_cast<uint16>(LogArgs.VisualLogVectorTwo.Y),
							LogArgs.bDrawWireframeValue,
							TEXT("%s"), *Message);
	#else
						FVisualLogger::CircleLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne,
							LogArgs.VisualLogVectorTwo.X,
							LogArgs.VisualLogShapeColorValue,
							static_cast<uint16>(LogArgs.VisualLogVectorTwo.Y),
							TEXT("%s"), *Message);
	#endif
						break;
					}
				case DbgLogArgs::EDbgVisualLogShape::Capsule:
					{
	#if UE_VERSION_NEWER_THAN(5, 4, 0)
						FVisualLogger::CapsuleLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne.X,
							LogArgs.VisualLogVectorOne.Y,
							LogArgs.VisualLogShapeRotationValue.Quaternion(),
							LogArgs.VisualLogShapeColorValue,
							LogArgs.bDrawWireframeValue,
							TEXT("%s"), *Message);
	#else
						FVisualLogger::GeometryShapeLogf(
							LogArgs.VisualLoggerOwnerValue,
							*LogCategory, LogArgs.VerbosityValue,
							LogArgs.VisualLogShapeLocationValue,
							LogArgs.VisualLogVectorOne.X,
							LogArgs.VisualLogVectorOne.Y,
							LogArgs.VisualLogShapeRotationValue.Quaternion(),
							LogArgs.VisualLogShapeColorValue,
							TEXT("%s"), *Message);
	#endif
						break;
					}
			}
		}
#endif
		if(LogArgs.VisualLoggerOwnerValue && LogArgs.bOnlyUseVisualLogger)
		{
			return;
		}
		
		
		switch (LogArgs.VerbosityValue)
		{
		case ELogVerbosity::Display:
			{
				if(LogArgs.ScreenColorValue == FColor::Transparent)
				{
					LogArgs.ScrnColor(FColor::White);
				}
				
				if(LogArgs.ScreenDurationValue < 0)
				{
					LogArgs.ScrnDuration(10);
				}
				
				if (LogArgs.bLogToSlateNotify)
				{
					FNotificationInfo Info{FText::FromString(Message)};
					Info.ExpireDuration = 6.f;
					FSlateNotificationManager::Get().AddNotification(Info);
				}

				if (LogArgs.bLogToEditorMessageLog)
				{
					if (LogArgs.bShowEditorMessageLogImmediately)
					{
						FMessageLog MsgLog(LogCategory->GetCategoryName());
						MsgLog.Info(FText::FromString(Message));
						MsgLog.Open(EMessageSeverity::Type::Info);
					}
					else
					{
						// Piggy back of PIE which will auto open the message log if any entries exist.
						FMessageLog("PIE").Info(FText::FromString(Message));
					}
				}
				break;
			}
		case ELogVerbosity::Warning:
			{
				if(LogArgs.ScreenColorValue == FColor::Transparent)
				{
					LogArgs.ScrnColor(FColor::Yellow);
				}
				
				if(LogArgs.ScreenDurationValue < 0)
				{
					LogArgs.ScrnDuration(20);
				}
				
				if (LogArgs.bLogToSlateNotify)
				{
					FNotificationInfo Info{FText::FromString(Message)};
					Info.ExpireDuration = 15.f;
					FSlateNotificationManager::Get().AddNotification(Info);
				}

				if (LogArgs.bLogToEditorMessageLog)
				{
					if (LogArgs.bShowEditorMessageLogImmediately)
					{
						FMessageLog MsgLog(LogCategory->GetCategoryName());
						MsgLog.Warning(FText::FromString(Message));
						MsgLog.Open(EMessageSeverity::Type::Warning);
					}
					else
					{
						// Piggy back of PIE which will auto open the message log if any entries exist.
						FMessageLog("PIE").Warning(FText::FromString(Message));
					}
				}
				break;
			}
		case ELogVerbosity::Error:
			{
				if(LogArgs.ScreenColorValue == FColor::Transparent)
				{
					LogArgs.ScrnColor(FColor::Red);
				}
				
				if(LogArgs.ScreenDurationValue < 0)
				{
					LogArgs.ScrnDuration(30);
				}
				
				if (LogArgs.bLogToSlateNotify)
				{
					FNotificationInfo Info{FText::FromString(Message)};
					Info.ExpireDuration = 30.f;
					FSlateNotificationManager::Get().AddNotification(Info);
				}

				if (LogArgs.bLogToEditorMessageLog)
				{
					if (LogArgs.bShowEditorMessageLogImmediately)
					{
						FMessageLog MsgLog(LogCategory->GetCategoryName());
						MsgLog.Error(FText::FromString(Message));
						MsgLog.Open(EMessageSeverity::Type::Error);
					}
					else
					{
						// Piggy back of PIE which will auto open the message log if any entries exist.
						FMessageLog("PIE").Error(FText::FromString(Message));
					}
				}
				break;
			}
		case ELogVerbosity::Fatal:
			{
				if(LogArgs.ScreenColorValue == FColor::Transparent)
				{
					LogArgs.ScrnColor(FColor::Blue);
				}
				
				if(LogArgs.ScreenDurationValue < 0)
				{
					LogArgs.ScrnDuration(30);
				}

				if (LogArgs.bLogToSlateNotify)
				{
					FNotificationInfo Info{FText::FromString(Message)};
					Info.ExpireDuration = 30.f;
					FSlateNotificationManager::Get().AddNotification(Info);
				}

				if (LogArgs.bLogToEditorMessageLog)
				{
					if (LogArgs.bShowEditorMessageLogImmediately)
					{
						FMessageLog MsgLog(LogCategory->GetCategoryName());
						MsgLog.Error(FText::FromString(Message));
						MsgLog.Open(EMessageSeverity::Type::Error);
					}
					else
					{
						// Piggy back of PIE which will auto open the message log if any entries exist.
						FMessageLog("PIE").Error(FText::FromString(Message));
					}
				}
				break;
			}
		default:
		case ELogVerbosity::Verbose:
			{
				if(LogArgs.ScreenColorValue == FColor::Transparent)
				{
					LogArgs.ScrnColor(FColor::White);
				}
				
				if(LogArgs.ScreenDurationValue < 0)
				{
					LogArgs.ScrnDuration(10);
				}

				if (LogArgs.bLogToSlateNotify)
				{
					FNotificationInfo Info{FText::FromString(Message)};
					Info.ExpireDuration = 6.f;
					FSlateNotificationManager::Get().AddNotification(Info);
				}

				if (LogArgs.bLogToEditorMessageLog)
				{
					if (LogArgs.bShowEditorMessageLogImmediately)
					{
						FMessageLog MsgLog(LogCategory->GetCategoryName());
						MsgLog.Info(FText::FromString(Message));
						MsgLog.Open(EMessageSeverity::Type::Info);
					}
					else
					{
						// Piggy back of PIE which will auto open the message log if any entries exist.
						FMessageLog("PIE").Info(FText::FromString(Message));
					}
				}
				break;
			}
		}


		if (LogArgs.bLogToMessageDialog)
		{
			EAppReturnType::Type Response = FMessageDialog::Open(LogArgs.AppMsgType,
				FText::FromString(Message), FText::FromName(LogCategory->GetCategoryName()));

			if (LogArgs.AppMessageResponse)
			{
				LogArgs.AppMessageResponse(Response);
			}
		}

		// Return early if we had no intention of logging to the screen/console
		if (LogArgs.bOnlyLogToSlateNotify
			|| LogArgs.bOnlyLogToMessageDialog
			|| (LogArgs.bLogToEditorMessageLog
				&& LogArgs.OutputDestinationValue == EDbgLogOutput::Con)) // The output message log already handles console logging for us.
		{
			return;
		}


		uint64 Key = 0;
		if (LogArgs.OutputDestinationValue != EDbgLogOutput::Con)
		{
			int PIEID = 0;
#if UE_VERSION_OLDER_THAN(5, 5, 0)
			PIEID = GPlayInEditorID;
#else
			PIEID = UE::GetPlayInEditorID();
#endif
			Key = Location.line() + PIEID + UniqueIdentifier +
				(LogArgs.ScreenKeyValue.IsSet() ? LogArgs.ScreenKeyValue.GetValue() : 0);
		}


		// This is basically UE_LOG but expanded so we dont need compile time log category stuff
		// also allows our logs to correctly associate with the dbgLOG macros location rather
		// than here in the function.
		static auto OutputLog = [](const std::source_location& Loc,
			const FLogCategoryBase& LC, ELogVerbosity::Type Verb,
			const FString& Msg)
		{
			static ::UE::Logging::Private::FStaticBasicLogDynamicData LOG_Dynamic;

#if bf_UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
			static ::UE::Logging::Private::FStaticBasicLogRecord LOG_Static(
				TEXT("%s"),
				Loc.file_name(),
				Loc.line(),
				&LOG_Dynamic);
			
			LOG_Static.File = Loc.file_name();
			LOG_Static.Line = Loc.line();

			if ((Verb & ELogVerbosity::VerbosityMask) == ::ELogVerbosity::Fatal)
			{
				::UE::Logging::Private::BasicFatalLog(LOG_Static, &LC);
			}
			else if ((Verb & ::ELogVerbosity::VerbosityMask) <= ::ELogVerbosity::VeryVerbose)
			{
				if ((Verb & ::ELogVerbosity::VerbosityMask) <= LC.GetCompileTimeVerbosity())
				{
					if (!LC.IsSuppressed(Verb))
					{
						FMsg::Logf(Loc.file_name(), Loc.line(), LC.GetCategoryName(), Verb, TEXT("%s"), *Msg);
					}
				}
			}
#else
			static ::UE::Logging::Private::FStaticBasicLogRecord LOG_Static(
				TEXT("%s"),
				Loc.file_name(),
				Loc.line(),
				Verb,
				LOG_Dynamic);

			LOG_Static.Verbosity = Verb;
			LOG_Static.File = Loc.file_name();
			LOG_Static.Line = Loc.line();

			if ((Verb & ELogVerbosity::VerbosityMask) == ::ELogVerbosity::Fatal)
			{
				::UE::Logging::Private::BasicFatalLog(LC, &LOG_Static, *Msg);
			}
			else if ((Verb & ::ELogVerbosity::VerbosityMask) <= ::ELogVerbosity::VeryVerbose)
			{
				if ((Verb & ::ELogVerbosity::VerbosityMask) <= LC.GetCompileTimeVerbosity())
				{
					if (!LC.IsSuppressed(Verb))
					{
						::UE::Logging::Private::BasicLog(LC, &LOG_Static, *Msg);
					}
				}
			}
#endif
		};

		
		switch (LogArgs.OutputDestinationValue)
		{
		case EDbgLogOutput::Con:
			{
				OutputLog(Location, (*LogCategory), LogArgs.VerbosityValue, Message);
				break;
			}
		case EDbgLogOutput::Scr:
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(Key, LogArgs.ScreenDurationValue,
						LogArgs.ScreenColorValue, Message, true);
				}
				break;
			}
		case EDbgLogOutput::Both:
			{
				// Only output log if we arent already writing it to the msg log since that handles console outputting.
				if (LogArgs.bLogToEditorMessageLog == false)
				{
					OutputLog(Location, (*LogCategory), LogArgs.VerbosityValue, Message);
				}
				
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(Key, LogArgs.ScreenDurationValue,
						LogArgs.ScreenColorValue, Message, true);
				}
				break;
			}
		}
	}




	
	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugCapsule(
		const UObject* WorldContextObject, const FVector& Center,
		float HalfHeight, float Radius, const FQuat& Rotation,
		const FColor& Color, bool bPersistentLines, float LifeTime,
		uint8 DepthPriority, float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugCapsule(
				W, Center, HalfHeight, Radius,
			    Rotation, Color, bPersistentLines, LifeTime,
			    DepthPriority, Thickness);
		}
			
		return *this;
	}

	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugCone(
		const UObject* WorldContextObject, const FVector& Origin,
		const FVector& Direction, float Length, float AngleWidth,
		float AngleHeight, int32 NumSides, const FColor& Color,
		bool bPersistentLines, float LifeTime,
		uint8 DepthPriority, float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugCone(
				W, Origin, Direction, Length,
			    FMath::DegreesToRadians(AngleWidth),
			    FMath::DegreesToRadians(AngleHeight),
			    NumSides, Color, bPersistentLines,
			    LifeTime, DepthPriority, Thickness);
				
		}
		return *this;
	}

	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugCylinder(
		const UObject* WorldContextObject, const FVector& Start,
		const FVector& End, float Radius,
		int32 Segments, const FColor& Color,
		bool bPersistentLines, float LifeTime,
		uint8 DepthPriority, float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugCylinder(
				W, Start, End, Radius,
			    Segments, Color, bPersistentLines,
			    LifeTime, DepthPriority, Thickness);
		}
		
			
		return *this;
	}

	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugDirectionalArrow(
		const UObject* WorldContextObject, const FVector& LineStart,
		const FVector& LineEnd, float ArrowSize, const FColor& Color,
		bool bPersistentLines, float LifeTime,
		uint8 DepthPriority, float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugDirectionalArrow(
				W, LineStart, LineEnd, ArrowSize,
			    Color, bPersistentLines, LifeTime,
			    DepthPriority, Thickness);
		}
		return *this;
	}

	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugLine(
		const UObject* WorldContextObject, const FVector& LineStart,
		const FVector& LineEnd, const FColor& Color,
		bool bPersistentLines, float LifeTime,
		uint8 DepthPriority, float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugLine(
				W, LineStart, LineEnd, Color,
			    bPersistentLines, LifeTime,
			    DepthPriority, Thickness);
		}
		return *this;
	}

	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugPoint(
		const UObject* WorldContextObject, const FVector& Position,
		float Size, const FColor& Color, bool bPersistentLines,
		float LifeTime, uint8 DepthPriority)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugPoint(
				W, Position, Size,
				Color, bPersistentLines,
				LifeTime, DepthPriority);
		}
		return *this;
	}

	
	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugSphere(
		const UObject* WorldContextObject, const FVector& Center,
		float Radius, int32 Segments, const FColor& Color,
		bool bPersistentLines, float LifeTime, uint8 DepthPriority,
		float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugSphere(
				W, Center, Radius, Segments,
			    Color, bPersistentLines, LifeTime,
			    DepthPriority, Thickness);
		}
		return *this;
	}
	

	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugString(
		const UObject* WorldContextObject, const FVector& TextLocation,
		const FString& Text, AActor* TestBaseActor,
		const FColor& TextColor, float Duration)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugString(
				W, TextLocation, Text,
				TestBaseActor, TextColor, Duration);
		}
		return *this;
	}

	inline DbgLogArgs::ThisClass& DbgLogArgs::DrawDebugBox(
		const UObject* WorldContextObject, const FVector& Center,
		const FVector& Extent, const FColor& Color,
		bool bPersistentLines, float LifeTime,
		uint8 DepthPriority, float Thickness)
	{
		if (const UWorld* W = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			::DrawDebugBox(
				W, Center, Extent, Color,
			    bPersistentLines, LifeTime,
			    DepthPriority, Thickness);
			
		}
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogText(
		const UObject* Owner, bool bOnlyLogVisually)
	{
		VisualLoggerOwnerValue = Owner;
		VisualLogShapeValue = EDbgVisualLogShape::None;
		bOnlyUseVisualLogger = bOnlyLogVisually;
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogSphere(
		const UObject* Owner, const FVector& Location,
		float Radius, FColor SphereColor,
		bool bDrawWireframe, bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Sphere;
			VisualLogShapeColorValue = SphereColor;
			
			VisualLogShapeLocationValue = Location;
			VisualLogVectorOne.X = Radius;

			bDrawWireframeValue = bDrawWireframe;
			bOnlyUseVisualLogger = bOnlyLogVisually;
		}
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogBox(
		const UObject* Owner, const FVector& MinExtent,
		const FVector& MaxExtent, const FVector& Location,
		const FRotator& Rotation, FColor BoxColor,
		bool bDrawWireframe, bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Box;
			VisualLogShapeColorValue = BoxColor;

			VisualLogShapeLocationValue = Location;
			VisualLogShapeRotationValue = Rotation;

			VisualLogVectorOne = MinExtent;
			VisualLogVectorTwo = MaxExtent;
				
			bDrawWireframeValue = bDrawWireframe;
			bOnlyUseVisualLogger = bOnlyLogVisually;
		}
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogCone(
		const UObject* Owner, const FVector& Location,
		const FVector& Direction, float Length, float Angle,
		FColor ConeColor, bool bDrawWireframe, bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Cone;
			VisualLogShapeColorValue = ConeColor;

			VisualLogShapeLocationValue = Location;

			VisualLogVectorOne = Direction;
			VisualLogVectorTwo.X = Length;
			VisualLogVectorTwo.Y = Angle;
				
			bDrawWireframeValue = bDrawWireframe;
			bOnlyUseVisualLogger = bOnlyLogVisually;
		}
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogLine(
		const UObject* Owner, const FVector& Start,
		const FVector& End, float Thickness,
		FColor LineColor,bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Line;
			VisualLogShapeColorValue = LineColor;

			VisualLogShapeLocationValue = Start;
			VisualLogVectorOne = End;
			VisualLogVectorTwo.X = Thickness;
				
			bOnlyUseVisualLogger = bOnlyLogVisually;
		}
		return *this;
	}


	inline DbgLogArgs& DbgLogArgs::VisualLogArrow(
		const UObject* Owner, const FVector& Start,
		const FVector& End, FColor ArrowColor,
		bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Arrow;
			VisualLogShapeColorValue = ArrowColor;

			VisualLogShapeLocationValue = Start;
			VisualLogVectorOne = End;
				
			bOnlyUseVisualLogger = bOnlyLogVisually;
		}
		return *this;
	}

		
	inline DbgLogArgs& DbgLogArgs::VisualLogDisk(
		const UObject* Owner, const FVector& Start,
		const FVector& UpDir, float Radius, FColor ArrowColor,
		uint16 Thickness, bool bOnlyLogVisually, bool bDrawWireframe)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Disk;
			VisualLogShapeColorValue = ArrowColor;

			VisualLogShapeLocationValue = Start;
			VisualLogVectorOne = UpDir;
			VisualLogVectorTwo.X = Radius;
			VisualLogVectorTwo.Y = Thickness;
				
			bOnlyUseVisualLogger = bOnlyLogVisually;
			bDrawWireframeValue = bDrawWireframe;
		}
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogCapsule(
		const UObject* Owner, const FVector& Base,
		const FRotator& Rotation, float Radius,
		float HalfHeight, FColor CapsuleColor,
		bool bDrawWireframe, bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr)
		{
			VisualLoggerOwnerValue = Owner;
			VisualLogShapeValue = EDbgVisualLogShape::Capsule;
			VisualLogShapeColorValue = CapsuleColor;

			VisualLogShapeLocationValue = Base;
			VisualLogShapeRotationValue = Rotation;
			VisualLogVectorOne.X = HalfHeight;
			VisualLogVectorOne.Y = Radius;
				
			bDrawWireframeValue = bDrawWireframe;
			bOnlyUseVisualLogger = bOnlyLogVisually;
		}
		return *this;
	}

	
	inline DbgLogArgs& DbgLogArgs::VisualLogBounds(const UObject* Owner,
		const AActor* ActorToGetBoundsFrom, FColor BoundsColor,
		bool bDrawWireframe, bool bOnlyLogVisually)
	{
		// Only support a single visual log per macro.
		if (VisualLoggerOwnerValue == nullptr && ::IsValid(ActorToGetBoundsFrom))
		{
			FBox B = ActorToGetBoundsFrom->CalculateComponentsBoundingBoxInLocalSpace(); 
			VisualLogBox(Owner, B.Min, B.Max, ActorToGetBoundsFrom->GetActorLocation(), 
				ActorToGetBoundsFrom->GetActorRotation(),
				BoundsColor, bDrawWireframe, bOnlyLogVisually);
		}
		return *this;
	}



}

#else
	#define _INTERNAL_DBGLOGV(Args, Msg, Name, ...) 
	#define dbgLOG(Msg, ...)
	#define dbgLOGV(Args, Msg, ...)

#endif