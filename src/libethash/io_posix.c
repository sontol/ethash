/*
  This file is part of ethash.

  ethash is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ethash is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ethash.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file io_posix.c
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 */

#include "io.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>

FILE *ethash_fopen(const char *file_name, const char *mode)
{
    return fopen(file_name, mode);
}

char *ethash_strncat(char *dest, size_t dest_size, const char *src, size_t count)
{   
    return strlen(dest) + count + 1 <= dest_size ? strncat(dest, src, count) : NULL;
}

enum ethash_io_rc ethash_io_prepare(char const *dirname, ethash_h256_t seedhash)
{
    char read_buffer[DAG_MEMO_BYTESIZE];
    char expect_buffer[DAG_MEMO_BYTESIZE];
    enum ethash_io_rc ret = ETHASH_IO_FAIL;

    // assert directory exists, full owner permissions and read/search for others
    int rc = mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (rc == -1 && errno != EEXIST) {
        goto end;
    }

    char *memofile = ethash_io_create_filename(dirname, DAG_MEMO_NAME, sizeof(DAG_MEMO_NAME));
    if (!memofile) {
        goto end;
    }

    // try to open memo file
    FILE *f = ethash_fopen(memofile, "rb");
    if (!f) {
        // file does not exist, so no checking happens. All is fine.
        ret = ETHASH_IO_MEMO_MISMATCH;
        goto free_memo;
    }

    if (fread(read_buffer, 1, DAG_MEMO_BYTESIZE, f) != DAG_MEMO_BYTESIZE) {
        goto close;
    }

    ethash_io_serialize_info(REVISION, seedhash, expect_buffer);
    if (memcmp(read_buffer, expect_buffer, DAG_MEMO_BYTESIZE) != 0) {
        // we have different memo contents so delete the memo file
        if (unlink(memofile) != 0) {
            goto close;
        }
        ret = ETHASH_IO_MEMO_MISMATCH;
    }

    ret = ETHASH_IO_MEMO_MATCH;

close:
    fclose(f);
free_memo:
    free(memofile);
end:
    return ret;
}
