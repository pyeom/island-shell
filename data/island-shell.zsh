# island-shell hook for zsh: commands in the island.
#
#   source /usr/share/island-shell/island-shell.zsh    # in ~/.zshrc
#
# - Programs in ISLAND_WATCH (yay, pacman, make...) show in the island while
#   they run, with a button that focuses the terminal.
# - Any other command that takes ISLAND_NOTIFY_AFTER seconds or more shows
#   how it ended when it finishes.
# - Commands in ISLAND_IGNORE (editors, pagers, ssh...) never show.
#
# Set these before or after sourcing:
#   ISLAND_WATCH=(yay paru pacman make)   ISLAND_NOTIFY_AFTER=30
#   ISLAND_IGNORE+=(lazygit)

[[ -o interactive ]] || return 0
(( $+commands[island-run] )) || return 0

zmodload zsh/datetime
autoload -Uz add-zsh-hook

typeset -ga ISLAND_WATCH ISLAND_IGNORE
(( ${#ISLAND_WATCH} )) || ISLAND_WATCH=(yay paru pacman makepkg make ninja cmake cargo go npm pnpm yarn
    pip uv docker podman rsync cp mv dd tar zstd ffmpeg yt-dlp aria2c wget curl git flatpak)
(( ${#ISLAND_IGNORE} )) || ISLAND_IGNORE=(vim nvim vi nano hx emacs micro less more most man ssh mosh
    top htop btop nvtop tmux zellij screen watch tail journalctl claude codex lazygit ranger yazi nnn
    fzf python python3 node irb iex psql mysql sqlite3 sudoedit)
typeset -gi ISLAND_NOTIFY_AFTER=${ISLAND_NOTIFY_AFTER:-30}

typeset -g _island_id="" _island_start="" _island_title="" _island_marker=""
typeset -gi _island_count=0 _island_watched=0
typeset -g _island_dir=${XDG_RUNTIME_DIR:-/tmp}/island-shell-zsh
mkdir -p $_island_dir 2>/dev/null

# The program a command line runs: skips sudo, env, time, nice and
# VAR=value assignments.
_island_program() {
    local -a words=(${(z)1})
    local w
    for w in $words; do
        case $w in
        *=* | sudo | doas | env | time | nice | nohup | command | builtin | exec | noglob) continue ;;
        *) print -r -- ${w:t}; return ;;
        esac
    done
}

_island_preexec() {
    local program=$(_island_program "$1")
    _island_id="" _island_watched=0
    [[ -n $program ]] || return
    (( ${ISLAND_IGNORE[(Ie)$program]} )) && return
    _island_id="sh-$$-$(( ++_island_count ))"
    _island_start=$EPOCHREALTIME
    _island_title=$program
    (( ${ISLAND_WATCH[(Ie)$program]} )) || return
    # Shown once it has run for 2 s, so quick ones (git status) do not
    # flash by. The marker is gone when the command is.
    _island_watched=1
    _island_marker=$_island_dir/$_island_id
    : >| $_island_marker
    local id=$_island_id marker=$_island_marker args=${1#*$program}
    args=${${args## #}[1,80]}
    {
        sleep 2
        [[ -e $marker ]] || exit
        island-run --begin $id $program "$args"
        # Finished while it was being shown: precmd found nothing to update.
        [[ -e $marker ]] || islandctl end $id >/dev/null 2>&1
    } &!
}

_island_precmd() {
    local -i rc=$?
    [[ -n $_island_id ]] || return
    (( _island_watched )) && rm -f $_island_marker
    local -i took=$(( EPOCHREALTIME - _island_start ))
    if (( ISLAND_NOTIFY_AFTER > 0 && took >= ISLAND_NOTIFY_AFTER )); then
        island-run --end $_island_id $rc $took $_island_title &!
    elif (( _island_watched && took >= 2 )); then
        island-run --end $_island_id $rc $took &!
    fi
    _island_id=""
}

add-zsh-hook preexec _island_preexec
add-zsh-hook precmd _island_precmd
