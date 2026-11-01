#!/usr/bin/env python3
"""Build data/answers.txt and data/allowed.txt from the fserb/pt-br word lists.

Usage: build_wordlists.py source_folder exclusions.txt output_folder

The source folder has lexico, conjugações, icf and listas/negativas from
https://github.com/fserb/pt-br. Guesses accept every five-letter word from the
lexicon and the conjugations, plus plurals and feminines that really show up
in the corpus. Answers are the most common lexicon words, minus plurals,
offensive words and whatever is in the exclusions file.
"""

import re
import sys
import unicodedata

WORD_LEN = 5
MAX_ANSWER_ICF = 15.0
UNKNOWN_ICF = 99.0
# The spelling reform dropped these accents and the trema.
OLD_SPELLING = re.compile(r"[éó]i[aeo]|ü")


def nfc(word):
    return unicodedata.normalize("NFC", word)


def plain(word):
    decomposed = unicodedata.normalize("NFD", word)
    return "".join(c for c in decomposed if not unicodedata.combining(c))


def fits(word):
    letters = plain(word)
    return (
        len(letters) == WORD_LEN and letters.isascii() and letters.isalpha() and letters.islower()
    )


def read_lines(path):
    with open(path, encoding="utf-8") as f:
        return [nfc(line.strip()) for line in f if line.strip()]


def read_icf(path):
    icf = {}
    for line in read_lines(path):
        word, _, score = line.rpartition(",")
        icf[word] = float(score)
    return icf


def singulars(word):
    """Forms a plural could come from, leaving accents to the lookup."""
    if not word.endswith("s"):
        return []
    forms = [word[:-1]]
    if word.endswith("es"):
        forms.append(word[:-2])
    if word.endswith(("ões", "ães", "ãos")):
        forms.append(word[:-3] + "ão")
    if word.endswith("ns"):
        forms.append(word[:-2] + "m")
    if word.endswith("is"):
        forms += [word[:-2] + "l", word[:-3] + "il"]
    return forms


def bases(word):
    """Dictionary forms an inflected word could come from."""
    forms = [word] + singulars(word)
    forms += [f[:-1] + "o" for f in forms if f.endswith("a")]
    return forms


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)

    source, exclusions, output = sys.argv[1:]
    full_lexicon = read_lines(f"{source}/lexico")
    lexicon = {w for w in full_lexicon if fits(w)}
    conjugations = {w for w in read_lines(f"{source}/conjugações") if fits(w)}
    icf = read_icf(f"{source}/icf")
    offensive = {plain(w).lower() for w in read_lines(f"{source}/listas/negativas")}
    excluded = {plain(w) for w in read_lines(exclusions) if not w.startswith("#")}

    # Accents are matched loosely since "meses" comes from "mês".
    known = {plain(w) for w in full_lexicon}
    inflected = {
        w
        for w in icf
        if fits(w)
        and w not in lexicon
        and w not in conjugations
        and any(plain(b) in known for b in bases(w)[1:])
    }

    forms = {}
    for word in lexicon | conjugations | inflected:
        forms.setdefault(plain(word), []).append(word)

    # One spelling per word: current spelling first, then the most common.
    best = {}
    for key, words in forms.items():
        best[key] = min(
            words, key=lambda w: (bool(OLD_SPELLING.search(w)), icf.get(w, UNKNOWN_ICF))
        )

    answers = []
    for word in sorted(lexicon, key=lambda w: (icf.get(w, UNKNOWN_ICF), w)):
        key = plain(word)
        if icf.get(word, UNKNOWN_ICF) > MAX_ANSWER_ICF or best[key] != word:
            continue
        if key in offensive or key in excluded or OLD_SPELLING.search(word):
            continue
        if any(plain(s) in known for s in singulars(word)):
            continue
        answers.append(word)

    answer_keys = {plain(w) for w in answers}
    allowed = sorted(w for key, w in best.items() if key not in answer_keys)

    with open(f"{output}/answers.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(answers) + "\n")
    with open(f"{output}/allowed.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(allowed) + "\n")
    print(f"{len(answers)} answers, {len(allowed)} other guesses")


if __name__ == "__main__":
    main()
