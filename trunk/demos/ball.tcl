# ball.tcl - Copyright (C) 2006 Pat Thoyts <patthoyts@users.sourceforge.net>
#
# SDL Demo: Displays a bouncing ball animation.
#
# $Id$

if {[file exists [file join Release tclsdl.dll]]} {
    load [file join Release tclsdl.dll]
}

package require Tclsdl

namespace eval App {
    variable uid 0
}

proc ::sdl::onEvent {type args} {
    variable ::App::App
    switch -exact -- $type {
        Quit {
            set ::App::App(forever) 1
        }
        Configure {
            foreach {w h} $args break
            set App(width) $w
            set App(height) $h
            $App(screen) configure -width $w -height $h
        }
        Motion {}
        Enter {}
        Leave {}
        ButtonRelease {}
        ButtonPress {
            foreach {button x y} $args break
            if {$button == 1} {
                App::AddSprite
            }
        }                
        User {
            #foreach {what time} $args break
            #if {$time eq {}} {set time 0}
            #puts "$what [expr {[clock clicks -milliseconds] - $time}]"
        }
        default {
            puts "E: $type $args" 
        }
    }
}

proc App::Paint {} {
    variable App
    #$App(bg) blit $App(screen) 0 0
    $App(screen) fill 0x00c0c0c0
    foreach Sprite $App(sprites) {
        upvar #0 $Sprite sprite
        $sprite(surface) blit $App(screen) $sprite(pos_x) $sprite(pos_y) \
            [lindex $sprite(regions) $sprite(region_index)]
    }
    $App(screen) flip
    return
}

proc App::Update {} {
    variable App
    foreach Sprite $App(sprites) {
        upvar #0 $Sprite sprite
        foreach {dx dy} $sprite(vector) break
        incr sprite(pos_x) $dx
        incr sprite(pos_y) $dy
        set ow 0
        if {$sprite(pos_x) < 1} {
            set sprite(vector) [list $sprite(speed) $dy]
            set ow 1
        } elseif {$sprite(pos_x) > ($App(width) - 82)} {
            set sprite(vector) [list -$sprite(speed) $dy]
            set ow 1
        }
        if {$sprite(pos_y) < 1} {
            set sprite(vector) [list $dx $sprite(speed)]
            set ow 1
        } elseif {$sprite(pos_y) > ($App(height) - 82)} {
            set sprite(vector) [list $dx -$sprite(speed)]
            set ow 1
        }
        if {$ow} {
            if {$sprite(aid) ne {}} {
                after cancel $sprite(aid)
            }
            set sprite(region_index) 4
            set sprite(aid) [after 150 \
                                 [list [namespace origin UpdateFace] $Sprite]]
            sdl::event "<<Bump>>" [clock clicks -milliseconds]
            if {[info exists App(beep)]} {
                #if {[$App(beep) playing]} { $App(beep) halt }
                $App(beep) play -channel 1
            }
        }
    }
    Paint
    after 30 [list [namespace origin Update]]
}

proc App::UpdateFace {Sprite} {
    upvar #0 $Sprite sprite
    foreach {dx dy} $sprite(vector) break
    if {$dx < 0} {
        set sprite(region_index) [expr {($dy < 0) ? 1 : 2}]
    } else {
        set sprite(region_index) [expr {($dy < 0) ? 0 : 3}]
    }
    set sprite(aid) {}
}        
    
proc App::CreateSprite {surface} {
    variable uid
    set Sprite [namespace current]::sprite[incr uid]
    upvar #0 $Sprite sprite
    array set sprite {aid {}}
    set sprite(surface) $surface
    set sprite(pos_x) [expr {int(rand() * 32) * 20}]
    set sprite(pos_y) [expr {int(rand() * 24) * 10}]
    set sprite(speed) [expr {int(rand() * 12) + 3}]
    set dir [expr {(rand() < 0.5) ? 1 : -1}]
    set sprite(vector) [list [expr {$dir * $sprite(speed)}] $sprite(speed)]
    set sprite(region_index) 0
    set sprite(regions) [list {0 0 93 82} {94 0 84 82} {179 0 82 82} \
                             {262 0 86 82} { 348 0 89 82}]
    return $Sprite
}

proc App::AddSprite {} {
    variable App
    lappend App(sprites) [CreateSprite $App(faces)]
}

proc App::Main {} {
    variable App
    set dir [file dirname [info script]]
    set App(width) 640
    set App(height) 480
    set App(screen) [sdl::surface -resizable \
                         -width $App(width) -height $App(height)]
    #set App(bg) [sdl::surface -bitmap [file join $dir images ball_bg.bmp]]
    set App(faces) [sdl::surface -bitmap [file join $dir images faces.bmp]]
    $App(faces) setcolorkey 0x00ff00ff

    if {0} {
        sdl::mixer init -frequency 11025
        set App(beep) [sdl::mixer load -type sample \
                           [file join $dir sounds doh.wav]]
        $App(beep) volume 10
    }

    set App(forever) 0
    set App(sprites) {}
    for {set n 0} {$n < 1} {incr n} { AddSprite }
    Paint
    after 20 [list [namespace origin Update]]
    vwait [namespace current]::App(forever)
}

if {!$tcl_interactive} {
    set r [catch [linsert $argv 0 App::Main] err]
    if {$r} {puts $::errorInfo} else {puts $err}
    exit
}