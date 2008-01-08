package require Tclsdl
package require Tk

set sprite {
0 0 0 0 0 1 1 1 1 1 0 0 0 0 0 0 
0 0 0 1 1 1 1 1 1 1 1 1 1 0 0 0 
0 0 1 1 1 1 1 1 1 1 1 1 1 1 0 0 
0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 
0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 
0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 
0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 
0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 
0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 
0 0 1 1 1 1 1 1 1 1 1 1 1 1 0 0 
0 0 0 1 1 1 1 1 1 1 1 1 1 0 0 0 
0 0 0 0 0 0 1 1 1 1 0 0 0 0 0 0
}

proc draw_sprite {screen x y color} {
    global sprite
    set idx 0
    foreach el $sprite {
        set ax [expr {$x+($idx%16)}]
        set ay [expr {$y+($idx/16)}]
        if {$el} {
            $screen pixel $ax $ay $color
        }
        incr idx
    }
}


proc blend_avg {source target} {
    lassign $source sourcer sourceb sourceg
    lassign $target targetr targetb targetg
    
    set targetr [expr {($sourcer+$targetr)/2}]
    set targetg [expr {($sourceg+$targetg)/2}]
    set targetb [expr {($sourceb+$targetb)/2}]
    return [list $targetr $targetb $targetg]
}


proc blend_mul {source target} {
    lassign $source sourcer sourceb sourceg
    lassign $target targetr targetb targetg
    
    set targetr [expr {($sourcer*$targetr)%256}]
    set targetg [expr {($sourceg*$targetg)%256}]
    set targetb [expr {($sourceb*$targetb)%256}]
    return [list $targetr $targetb $targetg]
}

proc blend_add {source target} {
    lassign $source sourcer sourceb sourceg
    lassign $target targetr targetb targetg
    
    set targetr [expr {$sourcer+$targetr}]
    set targetg [expr {$sourceg+$targetg}]
    set targetb [expr {$sourceb+$targetb}]
    
    if {$targetr>255} { set $targetr 255 }
    if {$targetg>255} { set $targetg 255 }
    if {$targetb>255} { set $targetb 255 }

    return [list $targetr $targetb $targetg]
}

proc scaleblit {} {
    for {set i 0} {$i < 480} {incr i} {
        for {set j 0} {$j < 640} {incr j} {
            set cy [expr {(int($i*0.95)+12)}]
            set cx [expr {(int($i*0.95)+16)}]
            set color_screen [$::screen pixel $j $i]
            set color_temp [$::temp pixel $cx $cy]
            $::screen pixel $j $i [blend_avg $color_screen $color_temp]
        }
    }
}


proc render {} {
    set tick [clock milliseconds]
    for {set i 0} {$i < 128} {incr i} {
        set d [expr {$tick + $i*4}]
        set x [expr {int(320 + sin($d * 0.0034) * sin($d * 0.0134) * 300)}]
        set y [expr {int(240 + sin($d * 0.0033) * sin($d * 0.0234) * 220)}]
        set r [expr {int(sin(($tick*0.2+$i) * 0.234897)*127+128)}]
        set g [expr {int(sin(($tick*0.2+$i) * 0.123489)*127+128)}]
        set b [expr {int(sin(($tick*0.2+$i) * 0.312348)*127+128)}]
        draw_sprite $::screen $x $y [list $r $g $b]
        # $::screen blit $::temp 0 0
        # $::temp flip
        # scaleblit
        $::screen  flip
    }
}

proc ::sdl::onEvent {type args} {
    if {$type eq "Quit"} {
        exit
    }
}


set screen [sdl::surface -width 640 -height 480 -bpp 32]
set temp [sdl::surface -width 640 -height 480 -bpp 32]

wm withdraw .

proc loop {} {
    render
    after idle [list after 0 loop]
}

loop
vwait forever
