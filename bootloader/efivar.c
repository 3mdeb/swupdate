#include <unistd.h>
#include <string.h>
#include <efivar/efivar.h>
#include "bootloader.h"
#include "util.h"

#define EFI_CUSTOM_GUID EFI_GUID(0x8be4df61,0x93ca,0x11d2,0xaa0d,0x00,0xe0,0x98,0x03,0x2b,0x8d)

static const efi_guid_t GUID = EFI_CUSTOM_GUID;

static int do_env_set(const char *name, const char *value)
{
	uint32_t attributes = EFI_VARIABLE_NON_VOLATILE |
						  EFI_VARIABLE_BOOTSERVICE_ACCESS |
						  EFI_VARIABLE_RUNTIME_ACCESS;
	int ret = efi_set_variable(GUID, name, value, strlen(value), attributes, 0);
	if (ret)
		return ret;

	return 0;
}

static int do_env_unset(const char *name)
{
	int ret = efi_del_variable(GUID, name);
	if (ret)
		return ret;

	return 0;
}

static char *do_env_get(const char *name)
{
	size_t size;
	int ret;
	char *data = NULL;
	uint32_t attr;
	if ((ret = efi_get_variable(GUID, name, (uint8_t**)&data, &size, &attr)) || data == NULL) {
		ERROR("Couldn't get EFI variable. Return code = %d\n", ret);
		return NULL;
	}

	return data;
}

static int do_apply_list(const char *filename)
{
	FILE *fp = NULL;
	int ret = 0;
	char *line = NULL, *key = NULL, *value = NULL;
	size_t len = 0;

	fp = fopen(filename, "rb");
	if (!fp) {
		ERROR("Failed to open: %s\n", filename);
		ret = -1;
		goto cleanup;
	}

	while ((getline(&line, &len, fp)) != -1) {
		key = strtok(line, "=");
		value = strtok(NULL, "\t\n");
		if (value != NULL && key != NULL) {
			ret = do_env_set(key, value);
			if (ret) {
				ERROR("Setting pair [%s] = %s failed\n", key, value);
				goto cleanup;
			}
		}
	}
cleanup:
	if (fp)
		fclose(fp);
	if (line)
		free(line);
	return ret;
}

static bootloader efivar = {
	.env_get = &do_env_get,
	.env_set = &do_env_set,
	.env_unset = &do_env_unset,
	.apply_list = &do_apply_list
};

__attribute__((constructor))
static void efivar_probe(void)
{
	(void)register_bootloader(BOOTLOADER_EFIVAR, &efivar);
}
