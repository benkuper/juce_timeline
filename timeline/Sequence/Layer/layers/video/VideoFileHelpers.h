/*
  ==============================================================================

    VideoFileHelpers.h
    Created: 27 Sep 2026

    Shared helpers for the video layer file formats : which extensions are
    accepted by the import choosers, drag & drop and the libVLC backend, and
    how to tell still images apart from actual video files.

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

namespace VideoFileHelpers
{
	inline bool isVideoFile(const String& path)
	{
		const String ext = File(path).getFileExtension().toLowerCase();

		return ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".mkv"
			|| ext == ".wmv" || ext == ".webm" || ext == ".m4v";
	}

	inline bool isStillImageFile(const String& path)
	{
		const String ext = File(path).getFileExtension().toLowerCase();

		return ext == ".jpg" || ext == ".jpeg" || ext == ".jpe" || ext == ".jfif"
			|| ext == ".png" || ext == ".bmp" || ext == ".dib" || ext == ".gif"
			|| ext == ".tif" || ext == ".tiff" || ext == ".webp";
	}

	inline bool isVideoOrImageFile(const String& path)
	{
		return isVideoFile(path) || isStillImageFile(path);
	}

	inline String getSupportedVideoAndImageWildcards()
	{
		return "*.mp4;*.mov;*.avi;*.mkv;*.wmv;*.webm;*.m4v"
			";*.jpg;*.jpeg;*.jpe;*.jfif;*.png;*.bmp;*.dib;*.gif;*.tif;*.tiff;*.webp";
	}
}