package require Tclsdl

set dir [file dirname [info script]]

set screen [sdl::surface -width 640 -height 480 -bpp 32]
set message [sdl::surface -bitmap [file join $dir hello_world.bmp]]
set background [sdl::surface -bitmap [file join $dir background.bmp]]

$background blit $screen 0 0
$message blit $screen 180 140

$screen flip

sdl::wm caption "Hello world"

proc ::sdl::onEvent {type args} {
    if {$type eq "Quit"} {
        exit
    }
}

catch {wm withdraw .}
vwait forever
