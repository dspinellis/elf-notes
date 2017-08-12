/* is_dgsh.c --  Detect whether an executable is dgsh-compatible */

/* Copyright (C) 2017 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(__ELF_WORD_SIZE) && __ELF_WORD_SIZE == 64
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Nhdr Elf_Nhdr;
#else
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Nhdr Elf_Nhdr;
#endif

#define DGSH_NAME "DSpinellis/dgsh"

#define MAX_LINE_LEN 1024


/*
 * Return true if the provided ELF data contain a DGSH note section
 */
static int
has_dgsh_section(Elf_Shdr *shdr, char *strTab, int shNum, uint8_t *data)
{
  int   i;
  Elf_Nhdr *note;

  for (i = 0; i < shNum; i++)
    {
      if (strcmp(&strTab[shdr[i].sh_name], ".note.ident"))
	continue;
      note = (Elf_Nhdr *)(data + shdr[i].sh_offset);
      if (note->n_namesz == sizeof(DGSH_NAME) && memcmp(note + 1, DGSH_NAME, sizeof(DGSH_NAME)) == 0)
	return 1;
  }
  return 0;
}

/*
 * Return the position of string needle in the first line of the
 * (non-empty) haystack looking in up to hlen characters.
 */
static const char *
linenstr(const char *haystack, const char *needle, size_t hlen)
{
  size_t nlen = strlen(needle);
  while (nlen < hlen-- && *haystack != '\n')
    {
      if (memcmp(haystack, needle, nlen) == 0)
	return haystack;
      haystack++;
    }
  return (NULL);
}

/* Return true if a script's second line contains the sequence #!dgsh */
static int
is_magic_script_dgsh_program(const char *data, size_t len)
{
  const char magic[] = "#!dgsh";
  const char *line2 = memchr(data, '\n', len);

  if (line2 == NULL)
    return 0;
  len -= ++line2 - data;
  if (len < sizeof(magic))
    return 0;
  return (memcmp(line2, magic, sizeof(magic) - 1) == 0);
}


/* Return true if the ELF program pointed by data is dgsh-compatible */
static int
is_script_dgsh_program(void *data, size_t len)
{
  len = len < MAX_LINE_LEN ? len : MAX_LINE_LEN;

  return (linenstr(data, "dgsh-wrap", len) ||
  	  linenstr(data, "--dgsh", len) ||
  	  linenstr(data, "env dgsh", len) ||
	  is_magic_script_dgsh_program(data, len));
}

/* Return true if the ELF program pointed by data is dgsh-compatible */
static int
is_elf_dgsh_program(void *data)
{
  Elf_Ehdr *elf;
  Elf_Shdr *shdr;
  char *strtab;

  elf = (Elf_Ehdr *)data;
  shdr = (Elf_Shdr *)(data + elf->e_shoff);
  strtab = (char *)(data + shdr[elf->e_shstrndx].sh_offset);
  return has_dgsh_section(shdr, strtab, elf->e_shnum, (uint8_t*)data);
}

int
is_dgsh_program(const char *path)
{
  void *data;
  Elf_Ehdr *elf;
  Elf_Shdr *shdr;
  int fd;
  char *strtab;
  int r;
  off_t file_size;

  fd = open(path, O_RDONLY);
  if (fd == -1)
    return 0;
  file_size = lseek(fd, 0, SEEK_END);
  data = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (data == NULL)
    return 0;
  if (memcmp(data, "#!", 2) == 0)
    r = is_script_dgsh_program(data, file_size);
  else
    r = is_elf_dgsh_program(data);
  munmap(data, file_size);
  fprintf(stderr, "is_dgsh_program(%s)=%d\n", path, r)
  return r;
}

#ifdef DGSH_COMPAT
int
main(int argc, char *argv[])
{
  if (argc != 2)
    errx(1, "usage: %s program", argv[0]);
  return is_dgsh_program(argv[1]) ? 0 : 2;
}
#endif
