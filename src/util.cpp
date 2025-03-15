#include "util.h"
#include "menu.h"

#include <hk/diag/diag.h>

#include <nn/fs.h>

namespace cly::util {

void createFile(const sead::SafeString& filePath, s64 size, bool overwrite) {
	if (isFileExist(filePath)) {
		if (overwrite) {
			LOG_R(nn::fs::DeleteFile(filePath.cstr()));
		} else {
			return;
		}
	}

	LOG_R(nn::fs::CreateFile(filePath.cstr(), size));
}

bool isFileExist(const sead::SafeString& filePath) {
	nn::fs::DirectoryEntryType entryType;
	nn::fs::GetEntryType(&entryType, filePath.cstr());

	return entryType == nn::fs::DirectoryEntryType_File;
}

} // namespace cly::util
