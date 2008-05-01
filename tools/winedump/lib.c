/*
 *  Dump a COFF library (lib) file
 *
 * Copyright 2006 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "winedump.h"

static void dump_import_object(const IMPORT_OBJECT_HEADER *ioh)
{
    if (ioh->Version >= 1)
    {
#if 0 /* FIXME: supposed to handle uuid.lib but it doesn't */
        const ANON_OBJECT_HEADER *aoh = (const ANON_OBJECT_HEADER *)ioh;

        printf("CLSID {%08x-%04x-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}",
               aoh->ClassID.Data1, aoh->ClassID.Data2, aoh->ClassID.Data3,
               aoh->ClassID.Data4[0], aoh->ClassID.Data4[1], aoh->ClassID.Data4[2], aoh->ClassID.Data4[3],
               aoh->ClassID.Data4[4], aoh->ClassID.Data4[5], aoh->ClassID.Data4[6], aoh->ClassID.Data4[7]);
#endif
    }
    else
    {
        static const char * const obj_type[] = { "code", "data", "const" };
        static const char * const name_type[] = { "ordinal", "name", "no prefix", "undecorate" };
        const char *name;

        printf("  Version      : %X\n", ioh->Version);
        printf("  Machine      : %X (%s)\n", ioh->Machine, get_machine_str(ioh->Machine));
        printf("  TimeDateStamp: %08X %s\n", ioh->TimeDateStamp, get_time_str(ioh->TimeDateStamp));
        printf("  SizeOfData   : %08X\n", ioh->SizeOfData);
        name = (const char *)ioh + sizeof(*ioh);
        printf("  DLL name     : %s\n", name + strlen(name) + 1);
        printf("  Symbol name  : %s\n", name);
        printf("  Type         : %s\n", (ioh->Type < sizeof(obj_type)/sizeof(obj_type[0])) ? obj_type[ioh->Type] : "unknown");
        printf("  Name type    : %s\n", (ioh->NameType < sizeof(name_type)/sizeof(name_type[0])) ? name_type[ioh->NameType] : "unknown");
        printf("  %-13s: %u\n", (ioh->NameType == IMPORT_OBJECT_ORDINAL) ? "Ordinal" : "Hint", ioh->u.Ordinal);
        printf("\n");
    }
}

static void dump_long_import(const void *base, const IMAGE_SECTION_HEADER *ish, unsigned num_sect)
{
    unsigned i;
    const DWORD *imp_data5 = NULL;
    const WORD *imp_data6 = NULL;

    if (globals.do_dumpheader)
        printf("Section Table\n");

    for (i = 0; i < num_sect; i++)
    {
        if (globals.do_dumpheader)
            dump_section(&ish[i]);

        if (globals.do_dump_rawdata)
        {
            dump_data((const unsigned char *)base + ish[i].PointerToRawData, ish[i].SizeOfRawData, "    " );
            printf("\n");
        }

        if (!strcmp((const char *)ish[i].Name, ".idata$5"))
        {
            imp_data5 = (const DWORD *)((const char *)base + ish[i].PointerToRawData);
        }
        else if (!strcmp((const char *)ish[i].Name, ".idata$6"))
        {
            imp_data6 = (const WORD *)((const char *)base + ish[i].PointerToRawData);
        }
        else if (globals.do_debug && !strcmp((const char *)ish[i].Name, ".debug$S"))
        {
            const char *imp_debug$ = (const char *)base + ish[i].PointerToRawData;

            codeview_dump_symbols(imp_debug$, ish[i].SizeOfRawData);
            printf("\n");
        }
    }

    if (imp_data5)
    {
        WORD ordinal = 0;
        const char *name = NULL;

        if (imp_data5[0] & 0x80000000)
            ordinal = (WORD)(imp_data5[0] & ~0x80000000);

        if (imp_data6)
        {
            if (!ordinal) ordinal = imp_data6[0];
            name = (const char *)(imp_data6 + 1);
        }
        else
        {
            /* FIXME: find out a name in the section's data */
        }

        if (ordinal)
        {
            printf("  Symbol name  : %s\n", name ? name : "(ordinal import) /* FIXME */");
            printf("  %-13s: %u\n", (imp_data5[0] & 0x80000000) ? "Ordinal" : "Hint", ordinal);
            printf("\n");
        }
    }
}

enum FileSig get_kind_lib(void)
{
    const char*         arch = PRD(0, IMAGE_ARCHIVE_START_SIZE);
    if (arch && !strncmp(arch, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE))
        return SIG_COFFLIB;
    return SIG_UNKNOWN;
}

void lib_dump(void)
{
    long cur_file_pos;
    const IMAGE_ARCHIVE_MEMBER_HEADER *iamh;

    cur_file_pos = IMAGE_ARCHIVE_START_SIZE;

    for (;;)
    {
        const IMPORT_OBJECT_HEADER *ioh;
        long size;

        if (!(iamh = PRD(cur_file_pos, sizeof(*iamh)))) break;

        if (globals.do_dumpheader)
        {
            printf("Archive member name at %08lx\n", Offset(iamh));

            printf("Name %.16s", iamh->Name);
            if (!strncmp((const char *)iamh->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(iamh->Name)))
            {
                printf(" - %s archive linker member\n",
                       cur_file_pos == IMAGE_ARCHIVE_START_SIZE ? "1st" : "2nd");
            }
            else
                printf("\n");
            printf("Date %.12s %s\n", iamh->Date, get_time_str(strtoul((const char *)iamh->Date, NULL, 10)));
            printf("UserID %.6s\n", iamh->UserID);
            printf("GroupID %.6s\n", iamh->GroupID);
            printf("Mode %.8s\n", iamh->Mode);
            printf("Size %.10s\n\n", iamh->Size);
        }

        cur_file_pos += sizeof(IMAGE_ARCHIVE_MEMBER_HEADER);

        size = strtoul((const char *)iamh->Size, NULL, 10);
        size = (size + 1) & ~1; /* align to an even address */

        /* FIXME: only import library contents with the short format are
         * recognized.
         */
        if (!(ioh = PRD(cur_file_pos, sizeof(*ioh)))) break;
        if (ioh->Sig1 == IMAGE_FILE_MACHINE_UNKNOWN && ioh->Sig2 == IMPORT_OBJECT_HDR_SIG2)
        {
            dump_import_object(ioh);
        }
        else if (strncmp((const char *)iamh->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(iamh->Name)))
        {
            long expected_size;
            const IMAGE_FILE_HEADER *fh = (const IMAGE_FILE_HEADER *)ioh;

            if (globals.do_dumpheader)
            {
                dump_file_header(fh);
                if (fh->SizeOfOptionalHeader)
                {
                    const IMAGE_OPTIONAL_HEADER32 *oh = (const IMAGE_OPTIONAL_HEADER32 *)((const char *)fh + sizeof(*fh));
                    dump_optional_header(oh, fh->SizeOfOptionalHeader);
                }
            }
            /* Sanity check */
            expected_size = sizeof(*fh) + fh->SizeOfOptionalHeader + fh->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
            if (size > expected_size)
                dump_long_import(fh, (const IMAGE_SECTION_HEADER *)((const char *)fh + sizeof(*fh) + fh->SizeOfOptionalHeader), fh->NumberOfSections);
        }

        cur_file_pos += size;
    }
}
