set dir [file dirname [info script]]
set dll [file join ../tclsdl10[info sharedlibextension]]
if {![file exists $dll]} {
    error "Build Tclsdl first"
} else {
    load $dll
} 
package require Tk


scale .speed -variable speed -orient horizontal -from 0 -to 30
button .quit -text Quit -command exit

pack .speed
pack .quit
wm deiconify .

set VIDEOX 320
set VIDEOY 200

set TONES 256

set speed 4

proc gray_palette {} {
    global TONES
    for {set i 0} {$i < $TONES} {incr i 2} {
        lappend colors [list $i $i $i]
    }
    lappend colors {*}[lreverse $colors]
}

set screen [sdl::surface -width $VIDEOX -height $VIDEOY -bpp 8]
set palette [gray_palette]
$screen setcolors $palette 0

set title "Palette rotation"
sdl::wm caption $title
wm title . $title

proc init_buffer {screen} {
    global VIDEOX VIDEOY TONES
    for {set y 0} {$y < $VIDEOY} {incr y} {
        set row {}
        for {set x 0} {$x < $VIDEOX} {incr x} {
            set c1 [expr {int((sin(sqrt($y**2+$x**2)/8)+1)/2*$TONES)}]
            set c2 [expr {int((sin(sqrt($y**2+($VIDEOX-$x)**2)/8)+1)/2*$TONES)}]
            set c3 [expr {int((sin(sqrt(($VIDEOY-$y)**2+$x**2)/8)+1)/2*$TONES)}]
            set c4 [expr {int((sin(sqrt(($VIDEOY-$y)**2+($VIDEOX-$x)**2)/8)+1)/2*$TONES)}]
            $screen pixel $x $y [expr {($c1+$c2+$c3+$c4)/2}] 
        }
    }
}


proc rotate_palette {} {
    global palette
    set head [lindex $palette 0]
    set palette [lrange $palette 1 end]
    lappend palette $head
}

proc loop {speedvar body} {
    upvar 1 $speedvar speed
    uplevel 1 $body
    after idle [list after $speed [list loop $speedvar $body]]
}

proc draw {screen} {
    $screen setcolors [rotate_palette] 0
    $screen flip
}

init_buffer $screen
loop speed {
    draw $screen
}

proc ::sdl::onEvent {type args} {
    if {$type eq "Quit"} {
        exit
    }
}
