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

#include "DbgBlueprintFunctionLibrary.h"



void UDbgBlueprintFunctionLibrary::OutputLog(UObject* WorldContextObject,
    FName LogCategory,
    FString LogMessage,
    EDbgLogOutput LogOutput,
    EDbgLogVerbosity LogVerbosity,
    FDbgLogArgs LogExtraArgs)
{
#if KEEP_DBG_LOG
	ELogVerbosity::Type LV;
	switch (LogVerbosity)
	{
		default:
		case EDbgLogVerbosity::Display: LV = ELogVerbosity::Display; break;
		case EDbgLogVerbosity::Warning: LV = ELogVerbosity::Warning; break;
		case EDbgLogVerbosity::Error: LV = ELogVerbosity::Error; break;
	}

	DBG::Log::DbgLogArgs LogArgs;
	
	LogArgs.Category( LogCategory ).Verbosity( LV )
	.WCO( LogExtraArgs.bPrefixPIEInstanceInfo ? WorldContextObject : nullptr )
	.ScrnColor( LogExtraArgs.ScreenColor ).ScrnDuration( LogExtraArgs.ScreenDuration );

	switch (LogOutput)
	{
		case EDbgLogOutput::Con: LogArgs.Console(); break;
		case EDbgLogOutput::Scr: LogArgs.Screen(); break;
		case EDbgLogOutput::Both: LogArgs.ScreenAndConsole(); break;
	}

	if (LogExtraArgs.bLogDateAndTime)
	{
	    LogArgs.LogDateAndTime();
	}

	if (LogExtraArgs.bLogToSlateNotify)
	{
		LogArgs.LogToSlateNotify(LogExtraArgs.bOnlyLogToSlateNotify);
	}
	
	if (LogExtraArgs.bLogToEditorMessageLog)
	{
		LogArgs.LogToEditorMessageLog(LogExtraArgs.bShouldShowEditorMessageLogImmediately);
	}

	
	DBG::Log::Log(LogExtraArgs.ScreenKey != -1 ?
		LogExtraArgs.ScreenKey : FMath::Rand32(),
		std::source_location::current(), LogArgs,
		TEXT("{0}"), LogMessage);
	
#endif
}
