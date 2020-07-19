/*
 * AcpiPatcher: Patches FADT table to enable S0ix on supported platforms.
 *
 * Copyright 2018, Bingxing Wang. All rights reserved. <BR>
 * Portions copyright 2014-2018 Pete Batard <pete@akeo.ie>. <BR>
 *
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 * 
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 *
**/

#include <efi.h>
#include <efilib.h>
#include <IndustryStandard/Acpi51.h>

UINT8 SumBytes(const UINT8* arr, UINTN size)
{
	UINT8 sum = 0;
	for (UINTN i = 0; i < size; ++i) 
	{
		sum += arr[i];
	}
	return sum;
}

int VerifyAcpiRsdp2Checksums(const void* data) 
{
	const UINT8* arr = data;
	UINTN size = *(const UINT32*) &arr[20];
	return SumBytes(arr, 20) == 0 && SumBytes(arr, size) == 0;
}

int VerifyAcpiSdtChecksum(const void* data) 
{
	const UINT8* arr = data;
	UINTN size = *(const UINT32*)&arr[4];
	return SumBytes(arr, size) == 0;
}

void SetAcpiSdtChecksum(void* data) 
{
	UINT8* arr = data;
	UINTN size = *(const UINT32*)&arr[4];
	arr[9] = 0;
	arr[9] = -SumBytes(arr, size);
}

// Application entrypoint (must be set to 'efi_main' for gnu-efi crt0 compatibility)
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	UINTN Event;
	EFI_GUID Acpi20TableGuid = ACPI_20_TABLE_GUID;
	EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* Rsdp = NULL;
	EFI_ACPI_DESCRIPTION_HEADER* XsdtHeader = NULL;
	EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE* Fadt = NULL;

#if defined(_GNU_EFI)
	InitializeLib(ImageHandle, SystemTable);
#endif

	/*
	 * In addition to the standard %-based flags, Print() supports the following:
	 *   %N       Set output attribute to normal
	 *   %H       Set output attribute to highlight
	 *   %E       Set output attribute to error
	 *   %B       Set output attribute to blue color
	 *   %V       Set output attribute to green color
	 *   %r       Human readable version of a status code
	 */
	Print(L"\n%HAcpiPatcher 1.0.0%N\n\n");

	/* Check ACPI table's existence */
	for (UINTN i = 0; i < gST->NumberOfTableEntries; i++)
	{
		EFI_GUID* VendorGuid = &gST->ConfigurationTable[i].VendorGuid;
		if (!CompareGuid(VendorGuid, &AcpiTableGuid) && !CompareGuid(VendorGuid, &Acpi20TableGuid))
		{
			Print(L"%d: Not ACPI table \n", i);
			continue;
		}

		Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[i].VendorTable;
		if (Rsdp->Signature != EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE ||
			Rsdp->Revision < EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION || 
			!VerifyAcpiRsdp2Checksums(Rsdp))
		{
			Print(L"%d: Invalid ACPI RSDP table \n", i);
			Rsdp = NULL;
			continue;
		}

		Print(L"%d: RSDP Rev = %d \n", i, Rsdp->Revision);
		XsdtHeader = (EFI_ACPI_DESCRIPTION_HEADER*) (UINTN) Rsdp->XsdtAddress;

		if (XsdtHeader->Signature != EFI_ACPI_2_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE ||
			!VerifyAcpiSdtChecksum(XsdtHeader))
		{
			Print(L"%d: Invalid ACPI XSDT table \n", i);
			XsdtHeader = NULL;
			continue;
		}

		// Finished iteration
		break;
	}

	if (Rsdp != NULL && XsdtHeader != NULL)
	{
		UINT64* EntryAddress = (UINT64*)&XsdtHeader[1];
		UINT32 EntryArraySize = (XsdtHeader->Length - sizeof(*XsdtHeader)) / sizeof(UINT64);

		Print(L"XSDT: Count = %d\n", EntryArraySize);
		for (UINT32 j = 0; j < EntryArraySize; j++)
		{
			EFI_ACPI_DESCRIPTION_HEADER* Entry = (EFI_ACPI_DESCRIPTION_HEADER*)((UINTN)EntryAddress[j]);
			if (Entry->Signature != EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE)
			{
				Print(L"%d: Not FADT table \n", j);
				continue;
			}

			if (Entry->Revision < EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION)
			{
				Print(L"%d: FADT revision is below ACPI 5.0 \n", j);
				continue;
			}

			Print(L"FADT table located. \n");

			// Iteration completed
			Fadt = (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE*) Entry;
			break;
		}
	}
	
	if (Fadt != NULL)
	{
		Print(L"FADT Flags: 0x%x \n", Fadt->Flags);

		if ((Fadt->Flags >> 21) & 1U)
		{
			Print(L"S0 Low Power Idle State Flag is enabled on this platform \n");

			Print(L"Setting S0 Low Power Idle State Flag \n");

			// Low Power S0 Idle (V5) is bit 21, disable it
			Fadt->Flags &= ~(1UL<< 21);

			// Re-calc checksum
			Print(L"Setting new checksum \n");
			SetAcpiSdtChecksum(Fadt);

			Print(L"FADT patch completed. \n");
			Print(L"FADT Flags after patch: 0x%x \n", Fadt->Flags);
		}
		else
		{
			Print(L"S0 Low Power Idle State Flag is already disabled on this platform \n");
		}
	}

	Print(L"%EPress any key to exit.%N\n");
	SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
	SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Event);

	return EFI_SUCCESS;
}
