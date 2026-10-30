#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "save.h"

#define VERSION 2

static const char magic[4] = { 'Q', 'U', 'I', 'N' };

struct header {
    char magic[4];
    uint16_t version;
    uint16_t size;
    uint32_t checksum;
};

struct save save;

static const char *save_path;

// FNV-1a, enough to notice a truncated or damaged file.
static uint32_t checksum(const void *data, size_t size)
{
    const uint8_t *p = data;
    uint32_t hash = 2166136261u;

    while (size--) {
        hash ^= *p++;
        hash *= 16777619u;
    }
    return hash;
}

void save_defaults(struct save *s)
{
    memset(s, 0, sizeof(*s));
    s->sound_effects = 1;
    s->music = 1;
}

bool save_read(const char *path, struct save *s)
{
    struct header header;
    struct save loaded;
    FILE *f = fopen(path, "rb");

    save_defaults(s);
    if (!f)
        return false;

    bool ok = fread(&header, sizeof(header), 1, f) == 1 &&
              fread(&loaded, sizeof(loaded), 1, f) == 1 &&
              memcmp(header.magic, magic, sizeof(magic)) == 0 && header.version == VERSION &&
              header.size == sizeof(loaded) && header.checksum == checksum(&loaded, sizeof(loaded));
    fclose(f);

    if (ok)
        *s = loaded;
    return ok;
}

// Creates the folders leading to path, ignoring the ones that exist.
static void make_folders(const char *path)
{
    char folder[256];

    for (const char *p = strchr(path + 1, '/'); p; p = strchr(p + 1, '/')) {
        size_t length = p - path;
        if (length >= sizeof(folder))
            return;
        memcpy(folder, path, length);
        folder[length] = 0;
        mkdir(folder, 0777);
    }
}

bool save_write(const char *path, const struct save *s)
{
    struct header header = {
        .version = VERSION,
        .size = sizeof(*s),
        .checksum = checksum(s, sizeof(*s)),
    };

    memcpy(header.magic, magic, sizeof(magic));
    make_folders(path);
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;

    bool ok = fwrite(&header, sizeof(header), 1, f) == 1 && fwrite(s, sizeof(*s), 1, f) == 1;
    return fclose(f) == 0 && ok;
}

void save_load(const char *path)
{
    save_path = path;
    if (path)
        save_read(path, &save);
    else
        save_defaults(&save);
}

void save_commit(void)
{
    if (save_path)
        save_write(save_path, &save);
}

// Counts stop at 999 so they always fit on the stats screen.
static void count(uint16_t *n)
{
    if (*n < STATS_MAX)
        (*n)++;
}

void stats_record(struct stats *st, bool daily, int day, bool won, int guesses)
{
    if (daily) {
        if (st->played > 0 && day != st->last_day + 1)
            st->streak = 0;
        st->last_day = day;
    }

    count(&st->played);
    if (won) {
        count(&st->wins);
        count(&st->streak);
        if (st->streak > st->max_streak)
            st->max_streak = st->streak;
        count(&st->guesses[guesses - 1]);
    } else {
        st->streak = 0;
    }
}
