#!/usr/bin/env bash
# deploy.sh [demo ...|--all]      (deploy.sh -h for help)
#
# Publishes site/ to GitHub Pages and waits until it is live:
#   1. checks gh (installed, logged in), origin on GitHub, branch main not
#      behind origin, and Pages set to deploy from GitHub Actions
#   2. rebuilds the named demos (web), or every demo with --all
#   3. commits site/ only, and pushes main
#   4. watches the Pages workflow run for that commit
#   5. fetches each page's files from the live site and checks they match
#      the committed ones
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
. "$here/harness/demos.sh"
GL4ES_REPO=${GL4ES_REPO:-$(cd "$here/.." && pwd)/gl4es}

usage() {
    cat <<USAGE
usage: deploy.sh [demo ...|--all]

Publishes site/ to GitHub Pages and waits until the new pages are live.
With demo names, rebuilds those pages first (./build.sh <demo> web); --all
rebuilds every demo; with neither, deploys site/ as committed.

demos (default A -> B):
$(list_demos "$GL4ES_REPO" "$here")
USAGE
}

say()  { printf '\033[1m==> %s\033[0m\n' "$*"; }
die()  { printf 'deploy.sh: %s\n' "$*" >&2; exit 1; }

demos=()
for arg in "$@"; do
    case $arg in
    -h|--help) usage; exit 0 ;;
    --all)
        for d in "$here"/demos/*/; do [ -f "$d/main.c" ] && demos+=("$(basename "$d")"); done ;;
    -*) usage >&2; exit 1 ;;
    *)  [ -f "$here/demos/$arg/main.c" ] || die "no such demo: $arg (see deploy.sh -h)"
        demos+=("$arg") ;;
    esac
done

cd "$here"

# 1. Setup checks -------------------------------------------------------------
say "Checking setup"
command -v gh >/dev/null || die "gh (GitHub CLI) is not installed: https://cli.github.com (macOS: brew install gh)"
gh auth status -h github.com >/dev/null 2>&1 || die "gh is not logged in to github.com. Run: gh auth login"
repo=$(git remote get-url origin 2>/dev/null \
    | sed -nE 's#^(git@github\.com:|https://github\.com/)([^/]+/[^/.]+)(\.git)?$#\2#p')
[ -n "$repo" ] || die "origin is not a GitHub repository: $(git remote get-url origin 2>/dev/null || echo none)"
branch=$(git rev-parse --abbrev-ref HEAD)
[ "$branch" = main ] || die "on branch '$branch'; Pages deploys from main. Run: git switch main"
git fetch -q origin main
git merge-base --is-ancestor origin/main HEAD \
    || die "main is behind or has diverged from origin/main. Run: git pull --rebase"
[ -f .github/workflows/pages.yml ] || die ".github/workflows/pages.yml is missing"

pages=$(gh api "repos/$repo/pages" -q '.build_type + " " + .html_url' 2>/dev/null || true)
case $pages in
"")
    say "Pages is not enabled for $repo; enabling it (source: GitHub Actions)"
    gh api -X POST "repos/$repo/pages" -f build_type=workflow >/dev/null
    pages=$(gh api "repos/$repo/pages" -q '.build_type + " " + .html_url') ;;
workflow\ *) ;;
*)
    say "Pages builds from a branch; switching its source to GitHub Actions"
    gh api -X PUT "repos/$repo/pages" -f build_type=workflow >/dev/null
    pages=$(gh api "repos/$repo/pages" -q '.build_type + " " + .html_url') ;;
esac
url=${pages#* }
url=${url%/}
echo "    $repo, Pages at $url/"

# 2. Build --------------------------------------------------------------------
if [ ${#demos[@]} -gt 0 ]; then
    for d in "${demos[@]}"; do
        say "Building $d"
        "$here/build.sh" "$d" web | sed -n 1p
    done
fi

# 3. Commit site/ and push ----------------------------------------------------
if [ -n "$(git status --porcelain -- . ':!site')" ]; then
    echo "    note: changes outside site/ are not part of this deploy:"
    git status --short -- . ':!site' | sed 's/^/      /'
fi
if [ -n "$(git status --porcelain -- site)" ]; then
    say "Committing site/"
    git add -A site
    if [ ${#demos[@]} -gt 0 ]; then msg="site: rebuild $(IFS=,; echo "${demos[*]}" | sed "s/,/, /g")"; else msg="site: update"; fi
    git commit -q -m "$msg" -- site
    git log --oneline -1 | sed 's/^/    /'
fi
before=$(git rev-parse origin/main)
if [ "$(git rev-parse HEAD)" != "$before" ]; then
    say "Pushing main"
    git log --oneline "$before..HEAD" | sed 's/^/    /'
    git push -q origin main
else
    echo "    main is already pushed"
fi
sha=$(git rev-parse HEAD)

# 4. Watch the workflow -------------------------------------------------------
if git diff --quiet "$before" "$sha" -- site .github/workflows/pages.yml; then
    run=
    echo "    this push doesn't change site/, so no Pages run starts for it"
else
    say "Waiting for the Pages run for ${sha:0:10}"
    run=
    for _ in $(seq 30); do
        run=$(gh run list -R "$repo" --workflow pages.yml --commit "$sha" --limit 1 \
            --json databaseId -q '.[0].databaseId' 2>/dev/null || true)
        [ -n "$run" ] && break
        sleep 2
    done
    [ -n "$run" ] || die "no Pages run appeared for $sha; see https://github.com/$repo/actions"
    gh run watch "$run" -R "$repo" --exit-status --interval 5 >/dev/null \
        || die "the Pages run failed: https://github.com/$repo/actions/runs/$run"
    echo "    run $run succeeded"
fi

# 5. Verify the live site -----------------------------------------------------
if command -v sha256sum >/dev/null; then sum256() { sha256sum | cut -d' ' -f1; }
else sum256() { shasum -a 256 | cut -d' ' -f1; }; fi

# The files checked: the index, and each page's HTML and both .wasm builds.
files=(index.html)
for d in $(git ls-tree --name-only "$sha" site/ | sed 's#^site/##'); do
    [ -n "$(git ls-tree "$sha" "site/$d/index.html")" ] || continue
    files+=("$d/index.html" "$d/a/index.wasm" "$d/b/index.wasm")
done

stale() { # prints the files whose live copy differs from the commit
    local f
    for f in "${files[@]}"; do
        # The query string bypasses the CDN's cached copy.
        [ "$(curl -fsL "$url/$f?v=$sha" | sum256)" = "$(git show "$sha:site/$f" | sum256)" ] || echo "$f"
    done
}

say "Checking $url/ serves ${sha:0:10}"
for i in $(seq 18); do
    left=$(stale)
    [ -z "$left" ] && break
    if [ "$i" = 3 ] && [ -z "$run" ]; then
        echo "    live site is out of date; starting the Pages workflow"
        gh workflow run pages.yml -R "$repo" --ref main
        sleep 5
        run=$(gh run list -R "$repo" --workflow pages.yml --limit 1 --json databaseId -q '.[0].databaseId')
        gh run watch "$run" -R "$repo" --exit-status --interval 5 >/dev/null \
            || die "the Pages run failed: https://github.com/$repo/actions/runs/$run"
    fi
    sleep 10
done
[ -z "$left" ] || die "still not live after 3 minutes: $(echo $left)"

echo "    all ${#files[@]} files match"
say "Live:"
echo "    $url/"
for d in $(git ls-tree --name-only "$sha" site/ | sed 's#^site/##'); do
    [ -n "$(git ls-tree "$sha" "site/$d/index.html")" ] && echo "    $url/$d/"
done
exit 0
