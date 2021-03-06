(if (member "TERM=ansi" initial-environment)
    (progn
      (xterm-mouse-mode)
      (set-variable 'xterm-max-cut-length 2000)
      (define-key global-map "\e[L" [insert])
      (define-key global-map "\e[H" [home])
      (define-key global-map "\e[F" [end])
      (define-key global-map "\e[1;5A" [C-up])
      (define-key global-map "\e[1;5B" [C-down])
      (define-key global-map "\e[1;5C" [C-right])
      (define-key global-map "\e[1;5D" [C-left])
      (define-key global-map "\e[1;5F" [C-end])
      (define-key global-map "\e[1;5H" [C-home])
      (define-key global-map "\e[3;5~" 'kill-word)
      (define-key global-map "\e[7;5~" 'backward-kill-word)
      (define-key global-map "\eO5P" [C-f1])
      (define-key global-map "\eO5Q" [C-f2])
      (define-key global-map "\eO5R" [C-f3])
      (define-key global-map "\eO5S" [C-f4])
      (define-key global-map "\e[15;5~" [C-f5])
      (define-key global-map "\e[17;5~" [C-f6])
      (define-key global-map "\e[18;5~" [C-f7])
      (define-key global-map "\e[19;5~" [C-f8])
      (define-key global-map "\e[20;5~" [C-f9])
      (define-key global-map "\e[21;5~" [C-f10])
      (define-key global-map "\e[23;5~" [C-f11])
      (define-key global-map "\e[24;5~" [C-f12])
      (define-key global-map "\eO6P" [C-S-f1])
      (define-key global-map "\eO6Q" [C-S-f2])
      (define-key global-map "\eO6R" [C-S-f3])
      (define-key global-map "\eO6S" [C-S-f4])
      (define-key global-map "\e[15;6~" [C-S-f5])
      (define-key global-map "\e[17;6~" [C-S-f6])
      (define-key global-map "\e[18;6~" [C-S-f7])
      (define-key global-map "\e[19;6~" [C-S-f8])
      (define-key global-map "\e[20;6~" [C-S-f9])
      (define-key global-map "\e[21;6~" [C-S-f10])
      (define-key global-map "\e[23;6~" [C-S-f11])
      (define-key global-map "\e[24;6~" [C-S-f12])
      (define-key global-map "\eO3P" [M-f1])
      (define-key global-map "\eO3Q" [M-f2])
      (define-key global-map "\eO3R" [M-f3])
      (define-key global-map "\eO3S" [M-f4])
      (define-key global-map "\e[15;3~" [M-f5])
      (define-key global-map "\e[17;3~" [M-f6])
      (define-key global-map "\e[18;3~" [M-f7])
      (define-key global-map "\e[19;3~" [M-f8])
      (define-key global-map "\e[20;3~" [M-f9])
      (define-key global-map "\e[21;3~" [M-f10])
      (define-key global-map "\e[23;3~" [M-f11])
      (define-key global-map "\e[24;3~" [M-f12])
      (define-key global-map "\eO4P" [M-S-f1])
      (define-key global-map "\eO4Q" [M-S-f2])
      (define-key global-map "\eO4R" [M-S-f3])
      (define-key global-map "\eO4S" [M-S-f4])
      (define-key global-map "\e[15;4~" [M-S-f5])
      (define-key global-map "\e[17;4~" [M-S-f6])
      (define-key global-map "\e[18;4~" [M-S-f7])
      (define-key global-map "\e[19;4~" [M-S-f8])
      (define-key global-map "\e[20;4~" [M-S-f9])
      (define-key global-map "\e[21;4~" [M-S-f10])
      (define-key global-map "\e[23;4~" [M-S-f11])
      (define-key global-map "\e[24;4~" [M-S-f12])
      (define-key global-map "\e[1;2A" [S-up])
      (define-key global-map "\e[1;2B" [S-down])
      (define-key global-map "\e[1;2C" [S-right])
      (define-key global-map "\e[1;2D" [S-left])
      (define-key global-map "\e[1;2F" [S-end])
      (define-key global-map "\e[1;2H" [S-home])
      (define-key global-map "\e[3;2~" [S-delete])
      (define-key global-map "\e[5;2~" [S-prior])
      (define-key global-map "\e[6;2~" [S-next])
      (define-key global-map "\e[1;6A" [C-S-up])
      (define-key global-map "\e[1;6B" [C-S-down])
      (define-key global-map "\e[1;6C" [C-S-right])
      (define-key global-map "\e[1;6D" [C-S-left])
      (define-key global-map "\e[1;6F" [C-S-end])
      (define-key global-map "\e[1;6H" [C-S-home])
      (define-key global-map "\e[1;3A" [M-up])
      (define-key global-map "\e[1;3B" [M-down])
      (define-key global-map "\e[1;3C" [M-right])
      (define-key global-map "\e[1;3D" [M-left])
      (define-key global-map "\e[1;3F" [M-end])
      (define-key global-map "\e[1;3H" [M-home])
      (define-key global-map "\e[1;4A" [M-S-up])
      (define-key global-map "\e[1;4B" [M-S-down])
      (define-key global-map "\e[1;4C" [M-S-right])
      (define-key global-map "\e[1;4D" [M-S-left])
      (define-key global-map "\e[1;4F" [M-S-end])
      (define-key global-map "\e[1;4H" [M-S-home])
      (define-key global-map "\e[2;3~" [M-insert])
      (define-key global-map "\e[3;3~" [M-delete])
      (define-key global-map "\e[5;3~" [M-prior])
      (define-key global-map "\e[6;3~" [M-next])      
      (define-key global-map "\e[2;4~" [M-S-insert])
      (define-key global-map "\e[3;4~" [M-S-delete])
      (define-key global-map "\e[5;4~" [M-S-prior])
      (define-key global-map "\e[6;4~" [M-S-next])
      (define-key global-map "\e[2;6~" [C-S-insert])
      (define-key global-map "\e[3;6~" [C-S-delete])
      (define-key global-map "\e[5;6~" [C-S-prior])
      (define-key global-map "\e[6;6~" [C-S-next])
      (define-key global-map "\e[2;5~" [C-insert])
      (define-key global-map "\e[3;5~" [C-delete])
      (define-key global-map "\e[5;5~" [C-prior])
      (define-key global-map "\e[6;5~" [C-next])
      )
)
