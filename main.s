#========================================================
# Filename: main.s
#========================================================

#########################################################
#
# text segment
#
#########################################################
        .text
        #.globl main

#----------------------------------------------------------
# register usage:
# s0 - n (board size is nxn)
# s1 - base board pointer
# s2 - turn (1 = O (program), 2 = X (user))
# s3 - empty pieces left on board
# s4 - move result
#----------------------------------------------------------

#########################################################
# main
#########################################################
main:
        jal     start_game
        jal     game_loop

        li      $v0, 10                 # exit
        syscall

#########################################################
# read_int
#   Reads an integer from stdin using syscall 5.
#   Inputs:  -
#   Outputs: v0 = integer read
#   Clobbers: v0
#########################################################
read_int:
        li      $v0, 5
        syscall
        jr      $ra

#########################################################
# bool in_bounds(int r, int c)
#   a0 = r, a1 = c
#   v0 = 1 if 0 <= r < n and 0 <= c < n, else 0
#########################################################
in_bounds:
        bltz    $a0, IB_false           # r < 0 ? false
        bge     $a0, $s0, IB_false      # r >= n ? false
        bltz    $a1, IB_false           # c < 0 ? false
        bge     $a1, $s0, IB_false      # c >= n ? false
        li      $v0, 1                  # true
        jr      $ra
IB_false:
        move    $v0, $zero              # false
        jr      $ra

#########################################################
# int idx(int r, int c)
#   a0 = r, a1 = c
#   v0 = r*n + c
#########################################################
idx:
        mul     $t0, $a0, $s0           # t0 = r*n
        addu    $v0, $t0, $a1           # v0 = r*n + c
        jr      $ra

#########################################################
# bool is_valid_move(int r, int c)
#   a0 = r, a1 = c
#   v0 = 1 iff in_bounds(r,c) && board[r,c] == 0 else 0
#   Uses: $s0,$s1
#########################################################
is_valid_move:
        addiu   $sp, $sp, -16
        sw      $ra, 12($sp)
        sw      $a0,  8($sp)            # save r
        sw      $a1,  4($sp)            # save c

        # if (!in_bounds(r,c)) return 0
        jal     in_bounds
        beq     $v0, $zero, IVM_false

        # idx(r,c)
        lw      $a0,  8($sp)            # r
        lw      $a1,  4($sp)            # c
        jal     idx                     # v0 = r*n + c
        move    $t9, $v0                # save idx for debug

        # &board[r,c] = s1 + 4*idx ; cell = *addr
        addu    $t0, $v0, $v0           # t0 = 2*idx
        addu    $t0, $t0, $t0           # t0 = 4*idx
        addu    $t1, $s1, $t0           # t1 = address
        lw      $t2, 0($t1)             # t2 = board[r,c]

        # DEBUG: dump r,c,idx,addr,cell
        # la      $a0, DBG_IVM_R
        # li      $v0, 4
        # syscall
        # lw      $a0, 8($sp)
        # li      $v0, 1
        # syscall
        #
        # la      $a0, DBG_IVM_C
        # li      $v0, 4
        # syscall
        # lw      $a0, 4($sp)
        # li      $v0, 1
        # syscall
        #
        # la      $a0, DBG_IVM_IDX
        # li      $v0, 4
        # syscall
        # move    $a0, $t9
        # li      $v0, 1
        # syscall
        #
        # la      $a0, DBG_IVM_ADDR
        # li      $v0, 4
        # syscall
        # move    $a0, $t1
        # li      $v0, 1
        # syscall
        #
        # la      $a0, DBG_IVM_CELL
        # li      $v0, 4
        # syscall
        # move    $a0, $t2
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        bne     $t2, $zero, IVM_false

        li      $v0, 1                  # valid: empty
        b       IVM_done

IVM_false:
        move    $v0, $zero              # invalid

IVM_done:
        lw      $ra, 12($sp)
        addiu   $sp, $sp, 16
        jr      $ra

#########################################################
# int count_dir(int r, int c, int dr, int dc)
#   a0=r, a1=c, a2=dr, a3=dc
#   returns: v0 = number of contiguous cells equal to $s2
#            starting from (r+dr, c+dc) along (dr,dc)
#   Uses globals: $s0=n, $s1=board base, $s2=turn
#########################################################
count_dir:
        addiu   $sp, $sp, -32
        sw      $ra, 28($sp)
        sw      $a0, 24($sp)            # r
        sw      $a1, 20($sp)            # c
        sw      $a2, 16($sp)            # dr
        sw      $a3, 12($sp)            # dc
        move    $t7, $zero              # count = 0

        # DEBUG: TURN value at entry
        # la      $a0, DBG_CD_TURN
        # li      $v0, 4
        # syscall
        # move    $a0, $s2
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        # r += dr; c += dc
        lw      $t0, 24($sp)            # r
        lw      $t1, 16($sp)            # dr
        addu    $t0, $t0, $t1
        sw      $t0, 24($sp)

        lw      $t2, 20($sp)            # c
        lw      $t3, 12($sp)            # dc
        addu    $t2, $t2, $t3
        sw      $t2, 20($sp)

CD_Loop:
        # while (in_bounds(r,c) && board[idx(r,c)] == turn)
        lw      $a0, 24($sp)            # r
        lw      $a1, 20($sp)            # c
        jal     in_bounds
        beq     $v0, $zero, CD_Done

        lw      $a0, 24($sp)
        lw      $a1, 20($sp)
        jal     idx                     # v0 = r*n + c
        addu    $t4, $v0, $v0           # t4 = 2*idx
        addu    $t4, $t4, $t4           # t4 = 4*idx
        addu    $t5, $s1, $t4           # &board[r,c]
        lw      $t6, 0($t5)             # cell
        bne     $t6, $s2, CD_Done       # if cell is not player's piece, break

        addiu   $t7, $t7, 1             # ++count

        # step: r += dr; c += dc
        lw      $t0, 24($sp)            # r
        lw      $t1, 16($sp)            # dr
        addu    $t0, $t0, $t1
        sw      $t0, 24($sp)

        lw      $t2, 20($sp)            # c
        lw      $t3, 12($sp)            # dc
        addu    $t2, $t2, $t3
        sw      $t2, 20($sp)

        b       CD_Loop

CD_Done:
        move    $v0, $t7
        lw      $ra, 28($sp)
        addiu   $sp, $sp, 32
        jr      $ra

#########################################################
# bool is_winning_move(int r, int c)
#   a0 = r, a1 = c
#   returns v0 = 1 if placing at (r,c) completes a line of length n,
#           else 0
#   Uses: count_dir(r,c,dr,dc), globals $s0=n
#########################################################
is_winning_move:
        addiu   $sp, $sp, -16
        sw      $ra, 12($sp)
        sw      $a0,  8($sp)            # save r
        sw      $a1,  4($sp)            # save c

        # DEBUG: print n and TURN at entry
        # la      $a0, DBG_IWM_N
        # li      $v0, 4
        # syscall
        # move    $a0, $s0
        # li      $v0, 1
        # syscall
        # li      $a0, ' '                # space
        # li      $v0, 11
        # syscall
        # la      $a0, DBG_TURN
        # li      $v0, 4
        # syscall
        # move    $a0, $s2
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        # total = 1 + count_dir(r,c,0,1) + count_dir(r,c,0,-1)
        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, 0
        li      $a3, 1
        jal     count_dir
        move    $s5, $v0                # s5 = right

        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, 0
        li      $a3, -1
        jal     count_dir
        addiu   $s5, $s5, 1             # +1 for the placed stone
        addu    $s5, $s5, $v0           # total
        # DEBUG H
        # la      $a0, DBG_H
        # li      $v0, 4
        # syscall
        # move    $a0, $s5
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        beq     $s5, $s0, IWM_win

        # total = 1 + count_dir(r,c,1,0) + count_dir(r,c,-1,0)
        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, 1
        li      $a3, 0
        jal     count_dir
        move    $s5, $v0                # down

        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, -1
        li      $a3, 0
        jal     count_dir
        addiu   $s5, $s5, 1
        addu    $s5, $s5, $v0
        # DEBUG V
        # la      $a0, DBG_V
        # li      $v0, 4
        # syscall
        # move    $a0, $s5
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        beq     $s5, $s0, IWM_win

        # total = 1 + count_dir(r,c,1,1) + count_dir(r,c,-1,-1)
        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, 1
        li      $a3, 1
        jal     count_dir
        move    $s5, $v0                # down-right

        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, -1
        li      $a3, -1
        jal     count_dir
        addiu   $s5, $s5, 1
        addu    $s5, $s5, $v0
        # DEBUG D1
        # la      $a0, DBG_D1
        # li      $v0, 4
        # syscall
        # move    $a0, $s5
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        beq     $s5, $s0, IWM_win

        # total = 1 + count_dir(r,c,1,-1) + count_dir(r,c,-1,1)
        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, 1
        li      $a3, -1
        jal     count_dir
        move    $s5, $v0                # down-left

        lw      $a0, 8($sp)
        lw      $a1, 4($sp)
        li      $a2, -1
        li      $a3, 1
        jal     count_dir
        addiu   $s5, $s5, 1
        addu    $s5, $s5, $v0
        # DEBUG D2
        # la      $a0, DBG_D2
        # li      $v0, 4
        # syscall
        # move    $a0, $s5
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        beq     $s5, $s0, IWM_win

        # no line completed
        move    $v0, $zero
        b       IWM_done

IWM_win:
        li      $v0, 1

IWM_done:
        lw      $ra, 12($sp)
        addiu   $sp, $sp, 16
        jr      $ra

#########################################################
# bool is_draw()
#   v0 = 1 if no empty cells remain, else 0
#   Uses: $s3 (empty count)
#########################################################
is_draw:
        slti    $v0, $s3, 1            # v0 = (s3 <= 0) ? 1 : 0
        jr      $ra

#########################################################
# int make_move(int r, int c)
#   a0 = r, a1 = c
#   v0 = 0 win, 1 draw, 2 valid (continue), 3 invalid
#   Uses: $s0,$s1,$s2,$s3
#########################################################
make_move:
        addiu   $sp, $sp, -24
        sw      $ra, 20($sp)
        sw      $a0, 16($sp)            # save r
        sw      $a1, 12($sp)            # save c

        # if (!is_valid_move(r,c)) return 3;
        jal     is_valid_move
        beq     $v0, $zero, MM_invalid

        # i = idx(r,c)
        lw      $a0, 16($sp)            # r
        lw      $a1, 12($sp)            # c
        jal     idx                     # v0 = r*n + c

        # board[i] = turn;
        addu    $t0, $v0, $v0           # t0 = 2*i
        addu    $t0, $t0, $t0           # t0 = 4*i
        addu    $t1, $s1, $t0           # t1 = &board[i]
        sw      $s2, 0($t1)

        # --empty_left;
        addi    $s3, $s3, -1

        # if (is_winning_move(r,c)) return 0;
        lw      $a0, 16($sp)
        lw      $a1, 12($sp)
        jal     is_winning_move
        bne     $v0, $zero, MM_win

        # else if (is_draw()) return 1;
        jal     is_draw
        bne     $v0, $zero, MM_draw

        # else return 2;
        # DEBUG
        # la      $a0, DBG_MM_RET
        # li      $v0, 4
        # syscall
        # li      $t9, 2
        # move    $a0, $t9
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        li      $v0, 2
        b       MM_done

MM_invalid:
        # la      $a0, DBG_MM_RET
        # li      $v0, 4
        # syscall
        # li      $t9, 3
        # move    $a0, $t9
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        li      $v0, 3
        b       MM_done

MM_win:
        # la      $a0, DBG_MM_RET
        # li      $v0, 4
        # syscall
        # li      $t9, 0
        # move    $a0, $t9
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        li      $v0, 0
        b       MM_done

MM_draw:
        # la      $a0, DBG_MM_RET
        # li      $v0, 4
        # syscall
        # li      $t9, 1
        # move    $a0, $t9
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall
        li      $v0, 1

MM_done:
        lw      $ra, 20($sp)
        addiu   $sp, $sp, 24
        jr      $ra

#########################################################
# game_loop  [renamed from gameloop]
#   Runs the main turn-taking loop until win/draw.
#   Returns: -
#   Uses: $s2 (turn), calls user_move / cpu_move / print_board
#########################################################
game_loop:
        addiu   $sp, $sp, -4
        sw      $ra, 0($sp)

GL_While:
        # Whose turn? $s2 = 1 (CPU), 2 (user)
        li      $t0, 1
        beq     $s2, $t0, GL_CPU

        # user move
        jal     user_move
        b       GL_AfterMove

GL_CPU:
        jal     cpu_move

GL_AfterMove:
        move    $s4, $v0                # result = v0

        # DEBUG: RES=...
        # la      $a0, DBG_RES
        # li      $v0, 4
        # syscall
        # move    $a0, $s4
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        # Invalid => message, loop (no board print)
        li      $t2, 3
        beq     $s4, $t2, GL_Invalid

        # Valid => print board, then check outcome
        jal     print_board

        # Win?
        li      $t2, 0
        beq     $s4, $t2, GL_Win

        # Draw?
        li      $t2, 1
        beq     $s4, $t2, GL_Draw

        # Else (2): continue
        b       GL_While

GL_Invalid:
        la      $a0, INVALID
        li      $v0, 4
        syscall
        b       GL_While

GL_Win:
        # After a valid move, $s2 holds the NEXT player.
        # If NEXT == 2 (user), CPU just moved and won.
        li      $t3, 2
        beq     $s2, $t3, GL_CPUWon

        # Otherwise, player just won.
        la      $a0, WIN
        li      $v0, 4
        syscall
        b       GL_EndWhile

GL_CPUWon:
        la      $a0, LOSE               # "I'm the winner!"
        li      $v0, 4
        syscall
        b       GL_EndWhile

GL_Draw:
        la      $a0, DRAW
        li      $v0, 4
        syscall

GL_EndWhile:
        lw      $ra, 0($sp)
        addiu   $sp, $sp, 4
        jr      $ra

#########################################################
# user_move  [renamed from usermove]
#   Prompts, reads (r,c), attempts make_move(r,c).
#   On success (not invalid), sets next turn to CPU.
#   Returns: v0 = 0/1/2/3 (win/draw/valid/invalid)
#########################################################
user_move:
        addiu   $sp, $sp, -4
        sw      $ra, 0($sp)

        # DEBUG: show TURN at entry
        # la      $a0, DBG_USER_TURN
        # li      $v0, 4
        # syscall
        # move    $a0, $s2
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        # read r
        la      $a0, GETROW
        li      $v0, 4
        syscall
        jal     read_int
        move    $t0, $v0                # r

        # read c
        la      $a0, GETCOL
        li      $v0, 4
        syscall
        jal     read_int
        move    $t1, $v0                # c

        # result = make_move(r, c)
        move    $a0, $t0
        move    $a1, $t1
        jal     make_move               # v0 = 0/1/2/3

        # if (result != 3) turn = 1  (CPU next)
        li      $t2, 3
        beq     $v0, $t2, UM_skip_turn
        li      $s2, 1
UM_skip_turn:
        lw      $ra, 0($sp)
        addiu   $sp, $sp, 4
        jr      $ra

#########################################################
# bool check_line(int r0, int c0, int dr, int dc, int who, int &out_r, int &out_c)
#   a0=r0, a1=c0, a2=dr, a3=dc
#   5th arg (who)  : 16($sp) from caller  -> here at 80($sp)
#   6th arg (&out_r): 20($sp) from caller -> here at 84($sp)
#   7th arg (&out_c): 24($sp) from caller -> here at 88($sp)
# Globals: $s0 = n, $s1 = base of board (int*)
# Returns v0 = 1 if exactly one empty and the rest == who; also writes out_r/out_c
#########################################################
check_line:
        addiu   $sp, $sp, -64
        sw      $ra, 60($sp)

        # Save register args
        sw      $a0, 56($sp)            # r0
        sw      $a1, 52($sp)            # c0
        sw      $a2, 48($sp)            # dr
        sw      $a3, 44($sp)            # dc

        # Locals: cnt_who(40), er(36), ec(32), k(28), r(24), c(20)
        move    $t0, $zero
        sw      $t0, 40($sp)            # cnt_who = 0
        li      $t0, -1
        sw      $t0, 36($sp)            # er = -1
        sw      $t0, 32($sp)            # ec = -1
        move    $t0, $zero
        sw      $t0, 28($sp)            # k = 0

CL_For:
        # if (k >= n) break;
        lw      $t0, 28($sp)            # k
        slt     $t1, $t0, $s0
        beq     $t1, $zero, CL_AfterFor

        # r = r0 + k*dr
        lw      $t2, 56($sp)            # r0
        lw      $t3, 48($sp)            # dr
        mul     $t4, $t0, $t3           # k*dr
        addu    $t5, $t2, $t4           # r
        sw      $t5, 24($sp)

        # c = c0 + k*dc
        lw      $t2, 52($sp)            # c0
        lw      $t3, 44($sp)            # dc
        mul     $t4, $t0, $t3           # k*dc
        addu    $t6, $t2, $t4           # c
        sw      $t6, 20($sp)

        # cell = board[idx(r,c)]
        move    $a0, $t5
        move    $a1, $t6
        jal     idx                     # v0 = r*n + c
        addu    $t7, $v0, $v0           # t7 = 2*idx
        addu    $t7, $t7, $t7           # t7 = 4*idx
        addu    $t7, $t7, $s1           # &board[r,c]
        lw      $t8, 0($t7)             # cell

        # if (cell == who) { ++cnt_who; goto next; }
        lw      $t9, 80($sp)            # who
        beq     $t8, $t9, CL_CellIsWho

        # else if (cell == 0) { er=r; ec=c; goto next; }
        bne     $t8, $zero, CL_Opponent # cell != 0 -> opponent present => false
        lw      $t5, 24($sp)            # r
        lw      $t6, 20($sp)            # c
        sw      $t5, 36($sp)            # er = r
        sw      $t6, 32($sp)            # ec = c
        b       CL_Next

CL_CellIsWho:
        lw      $t2, 40($sp)            # cnt_who
        addiu   $t2, $t2, 1
        sw      $t2, 40($sp)
        b       CL_Next

CL_Opponent:
        # return false
        move    $v0, $zero
        b       CL_Epilogue

CL_Next:
        # ++k; loop
        lw      $t0, 28($sp)
        addiu   $t0, $t0, 1
        sw      $t0, 28($sp)
        b       CL_For

CL_AfterFor:
        # if (cnt_who == n-1) { *out_r=er; *out_c=ec; return true; } else false
        lw      $t2, 40($sp)            # cnt_who
        addiu   $t3, $s0, -1            # n-1
        bne     $t2, $t3, CL_False

        # write outs
        lw      $t4, 84($sp)            # &out_r
        lw      $t5, 36($sp)            # er
        sw      $t5, 0($t4)

        lw      $t4, 88($sp)            # &out_c
        lw      $t5, 32($sp)            # ec
        sw      $t5, 0($t4)

        li      $v0, 1
        b       CL_Epilogue

CL_False:
        move    $v0, $zero

CL_Epilogue:
        lw      $ra, 60($sp)
        addiu   $sp, $sp, 64
        jr      $ra

#########################################################
# bool find_winning_spot(int who, int &out_r, int &out_c)
#   a0 = who, a1 = &out_r, a2 = &out_c
#   Globals: $s0 = n, $s1 = board base
#   Returns: v0 = 1 if a winning spot found (out_r/out_c set), else 0
#########################################################
find_winning_spot:
        # Frame layout (48 bytes):
        #  44: $ra
        #  40: $s4 (who)
        #  36: $s5 (&out_r)
        #  32: $s6 (&out_c)
        #  28: $s7 (loop counter)
        #  16/20/24: outgoing args 5/6/7 for check_line
        addiu   $sp, $sp, -48
        sw      $ra, 44($sp)
        sw      $s4, 40($sp)
        sw      $s5, 36($sp)
        sw      $s6, 32($sp)
        sw      $s7, 28($sp)

        # Save our inputs
        move    $s4, $a0              # who
        move    $s5, $a1              # &out_r
        move    $s6, $a2              # &out_c

        ########################
        # 1) Rows: r = 0..n-1
        ########################
        move    $s7, $zero            # r = 0
FWS_RowLoop:
        slt     $t0, $s7, $s0         # r < n ?
        beq     $t0, $zero, FWS_Cols

        # check_line(r, 0, 0, 1, who, &out_r, &out_c)
        move    $a0, $s7              # r0 = r
        move    $a1, $zero            # c0 = 0
        move    $a2, $zero            # dr = 0
        li      $a3, 1                # dc = 1
        sw      $s4, 16($sp)          # who
        sw      $s5, 20($sp)          # &out_r
        sw      $s6, 24($sp)          # &out_c
        jal     check_line
        bne     $v0, $zero, FWS_True

        addiu   $s7, $s7, 1           # ++r
        b       FWS_RowLoop

        ########################
        # 2) Cols: c = 0..n-1
        ########################
FWS_Cols:
        move    $s7, $zero            # c = 0
FWS_ColLoop:
        slt     $t0, $s7, $s0         # c < n ?
        beq     $t0, $zero, FWS_MajorDiag

        # check_line(0, c, 1, 0, who, &out_r, &out_c)
        move    $a0, $zero            # r0 = 0
        move    $a1, $s7              # c0 = c
        li      $a2, 1                # dr = 1
        move    $a3, $zero            # dc = 0
        sw      $s4, 16($sp)
        sw      $s5, 20($sp)
        sw      $s6, 24($sp)
        jal     check_line
        bne     $v0, $zero, FWS_True

        addiu   $s7, $s7, 1           # ++c
        b       FWS_ColLoop

        ########################
        # 3) Major diagonal
        ########################
FWS_MajorDiag:
        # check_line(0, 0, 1, 1, who, &out_r, &out_c)
        move    $a0, $zero
        move    $a1, $zero
        li      $a2, 1
        li      $a3, 1
        sw      $s4, 16($sp)
        sw      $s5, 20($sp)
        sw      $s6, 24($sp)
        jal     check_line
        bne     $v0, $zero, FWS_True

        ########################
        # 4) Minor diagonal
        ########################
        # check_line(0, n-1, 1, -1, who, &out_r, &out_c)
        move    $a0, $zero
        addiu   $a1, $s0, -1          # c0 = n-1
        li      $a2, 1
        li      $a3, -1
        sw      $s4, 16($sp)
        sw      $s5, 20($sp)
        sw      $s6, 24($sp)
        jal     check_line
        bne     $v0, $zero, FWS_True

        # none found
        move    $v0, $zero
        b       FWS_Done

FWS_True:
        li      $v0, 1

FWS_Done:
        lw      $s7, 28($sp)
        lw      $s6, 32($sp)
        lw      $s5, 36($sp)
        lw      $s4, 40($sp)
        lw      $ra, 44($sp)
        addiu   $sp, $sp, 48
        jr      $ra

#########################################################
# int cpu_move()
#   Returns v0 = 0 win, 1 draw, 2 valid-continue, 3 invalid
#   Strategy: win-if-possible; else block; else first available.
#   Uses: find_winning_spot, is_valid_move, make_move; globals $s0,$s1,$s2
#########################################################
cpu_move:
        # frame: [ .. 36:$ra | 32.. ? | 24: r | 20: c | 16: saved rr | 12: saved cc ]
        addiu   $sp, $sp, -40
        sw      $ra, 36($sp)

        # DEBUG: cpumove entry + turn + board ptr
        # la      $a0, DBG_CPU_START
        # li      $v0, 4
        # syscall
        # move    $a0, $s2
        # li      $v0, 1
        # syscall
        # la      $a0, DBG_CPU_S1
        # li      $v0, 4
        # syscall
        # move    $a0, $s1
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        ########## 1) Try to win as CPU (who = 1) ##########
        li      $a0, 1                  # who = 1
        addiu   $a1, $sp, 24            # &r
        addiu   $a2, $sp, 20            # &c
        jal     find_winning_spot
        beq     $v0, $zero, CPU_Block   # if not found, try block

        # load r,c
        lw      $t0, 24($sp)            # r
        lw      $t1, 20($sp)            # c

        # DEBUG: WIN candidate (r,c)
        # la      $a0, DBG_CPU_WIN
        # li      $v0, 4
        # syscall
        # move    $a0, $t0
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t1
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_RPAREN
        # li      $v0, 4
        # syscall

        # print: "I choose (" r ", " c ")\n"
        la      $a0, CPU_CHOOSE1
        li      $v0, 4
        syscall
        move    $a0, $t0
        li      $v0, 1
        syscall
        la      $a0, CPU_COMMA
        li      $v0, 4
        syscall
        move    $a0, $t1
        li      $v0, 1
        syscall
        la      $a0, CPU_RPAREN
        li      $v0, 4
        syscall

        # DEBUG: call makemove with (r,c)
        # la      $a0, DBG_CALL_MM
        # li      $v0, 4
        # syscall
        # move    $a0, $t0
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t1
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_RPAREN
        # li      $v0, 4
        # syscall

        # result = make_move(r,c); turn = 2; return result
        move    $a0, $t0
        move    $a1, $t1
        jal     make_move               # v0 = result
        li      $s2, 2                  # player next
        b       CPU_Return

CPU_Block:
        ########## 2) Block player's win (who = 2) ##########
        li      $a0, 2
        addiu   $a1, $sp, 24
        addiu   $a2, $sp, 20
        jal     find_winning_spot
        beq     $v0, $zero, CPU_FirstAvail

        lw      $t0, 24($sp)            # r
        lw      $t1, 20($sp)            # c

        # DEBUG: BLOCK candidate (r,c)
        # la      $a0, DBG_CPU_BLOCK
        # li      $v0, 4
        # syscall
        # move    $a0, $t0
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t1
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_RPAREN
        # li      $v0, 4
        # syscall

        la      $a0, CPU_CHOOSE1
        li      $v0, 4
        syscall
        move    $a0, $t0
        li      $v0, 1
        syscall
        la      $a0, CPU_COMMA
        li      $v0, 4
        syscall
        move    $a0, $t1
        li      $v0, 1
        syscall
        la      $a0, CPU_RPAREN
        li      $v0, 4
        syscall

        # DEBUG: call makemove with (r,c)
        # la      $a0, DBG_CALL_MM
        # li      $v0, 4
        # syscall
        # move    $a0, $t0
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t1
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_RPAREN
        # li      $v0, 4
        # syscall

        move    $a0, $t0
        move    $a1, $t1
        jal     make_move               # v0 = result
        li      $s2, 2
        b       CPU_Return

CPU_FirstAvail:
        ########## 3) First available (row-major) ##########
        li      $t2, 0                  # rr = 0
CPU_RowLoop:
        slt     $t3, $t2, $s0           # rr < n ?
        beq     $t3, $zero, CPU_Draw    # none found -> draw
        li      $t4, 0                  # cc = 0
CPU_ColLoop:
        slt     $t5, $t4, $s0           # cc < n ?
        beq     $t5, $zero, CPU_NextRow

        # --- FIX: save rr/cc across jal is_valid_move (callee may clobber $t*) ---
        sw      $t2, 16($sp)            # save rr
        sw      $t4, 12($sp)            # save cc

        move    $a0, $t2                # is_valid_move(rr,cc)
        move    $a1, $t4
        jal     is_valid_move
        move    $t6, $v0                # keep return value

        # restore rr/cc
        lw      $t2, 16($sp)
        lw      $t4, 12($sp)

        # DEBUG: "CPU chk rr, cc ret <val>"
        # la      $a0, DBG_CPU_CHK
        # li      $v0, 4
        # syscall
        # move    $a0, $t2
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t4
        # li      $v0, 1
        # syscall
        # li      $a0, ' '
        # li      $v0, 11
        # syscall
        # move    $a0, $t6
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        beq     $t6, $zero, CPU_NextCol # if not valid, continue scanning

        # DEBUG: FIRST-AVAIL pick (rr,cc)
        # la      $a0, DBG_CPU_FIRST
        # li      $v0, 4
        # syscall
        # move    $a0, $t2
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t4
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_RPAREN
        # li      $v0, 4
        # syscall

        # choose (rr,cc)
        la      $a0, CPU_CHOOSE1
        li      $v0, 4
        syscall
        move    $a0, $t2
        li      $v0, 1
        syscall
        la      $a0, CPU_COMMA
        li      $v0, 4
        syscall
        move    $a0, $t4
        li      $v0, 1
        syscall
        la      $a0, CPU_RPAREN
        li      $v0, 4
        syscall

        # DEBUG: call makemove with (rr,cc)
        # la      $a0, DBG_CALL_MM
        # li      $v0, 4
        # syscall
        # move    $a0, $t2
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_COMMA
        # li      $v0, 4
        # syscall
        # move    $a0, $t4
        # li      $v0, 1
        # syscall
        # la      $a0, CPU_RPAREN
        # li      $v0, 4
        # syscall

        move    $a0, $t2                # make_move(rr,cc)
        move    $a1, $t4
        jal     make_move               # v0 = result
        li      $s2, 2
        b       CPU_Return

CPU_NextCol:
        addiu   $t4, $t4, 1
        b       CPU_ColLoop

CPU_NextRow:
        addiu   $t2, $t2, 1
        b       CPU_RowLoop

CPU_Draw:
        li      $v0, 1                  # draw

CPU_Return:
        lw      $ra, 36($sp)
        addiu   $sp, $sp, 40
        jr      $ra

#########################################################
# start_game
#   Prints greeting, reads n, initializes globals,
#   performs initial CPU open (init_board + print_board).
#   Inputs:  -
#   Outputs: $s0=n, $s1=BOARD, $s2=2 (user), $s3=n*n
#########################################################
start_game:
        addiu   $sp, $sp, -4
        sw      $ra, 0($sp)

        # opening message
        la      $a0, WELCOME
        li      $v0, 4
        syscall

        jal     read_int                # prompt user for board size n
        move    $s0, $v0                # store n in s0

        la      $s1, BOARD              # init board_pointe
        li      $s2, 2                  # turn = user
        mul     $t0, $s0, $s0           # t0 = n*n
        addu    $t1, $t0, $t0           # t1 = 2*n*n
        addu    $t1, $t1, $t1           # t1 = 4*n*n
        mul     $t2, $s0, $s0           # temp for empty count
        move    $s3, $t2                # empty_pieces = n*n

        # DEBUG: print n, bytes, board base
        # la      $a0, DBG_N
        # li      $v0, 4
        # syscall
        # move    $a0, $s0
        # li      $v0, 1
        # syscall
        # li      $a0, ' '
        # li      $v0, 11
        # syscall
        # la      $a0, DBG_BYTES
        # li      $v0, 4
        # syscall
        # move    $a0, $t1
        # li      $v0, 1
        # syscall
        # li      $a0, ' '
        # li      $v0, 11
        # syscall
        # la      $a0, DBG_BOARD
        # li      $v0, 4
        # syscall
        # move    $a0, $s1
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

        # cpu moves first
        la      $a0, FIRST
        li      $v0, 4
        syscall
        jal     init_board
        jal     print_board

        lw      $ra, 0($sp)
        addiu   $sp, $sp, 4
        jr      $ra


#########################################################
# init_board
#   Sets all board cells to 0; places CPU 'O' at center.
#   Updates $s3 (empty count) by -1.
#   Inputs:  $s0=n, $s1=BOARD
#   Outputs: board initialized; $s3 decremented
#########################################################
init_board:
        mul     $t0, $s0, $s0           # t0 = n^2

        # get board pointer
        move    $t1, $s1                # t1 = start
        addu    $t2, $t0, $t0           # t2 = 2n^2
        addu    $t2, $t2, $t2           # t2 = 4n^2 (bytes)
        addu    $t3, $t1, $t2           # t3 = end (start + 4*n^2)

        # DEBUG: clear-end address
        # la      $a0, DBG_CLR_END
        # li      $v0, 4
        # syscall
        # move    $a0, $t3
        # li      $v0, 1
        # syscall
        # li      $a0, 10
        # li      $v0, 11
        # syscall

IB_Loop:
        beq     $t1, $t3, IB_EndLoop
        sw      $0, 0($t1)
        addiu   $t1, $t1, 4
        b       IB_Loop
IB_EndLoop:
        # place 'O' in center
        addi    $t4, $s0, -1            # t4 = n-1
        li      $t5, 2                  # t5 = 2
        div     $t4, $t5                # LO = (n-1) / 2 (center)
        mflo    $t6                     # t6 = (n-1) / 2 (center)

        # idx = center * n + center
        mul     $t7, $t6, $s0           # t7 = center*n
        addu    $t7, $t7, $t6           # t7 = idx (elements)
        addu    $t7, $t7, $t7           # t7 = 2 * idx
        addu    $t7, $t7, $t7           # t7 = 4 * idx

        # board[idx] = 1
        addu    $t8, $s1, $t7           # addr = start + 4*idx
        li      $t5, 1
        sw      $t5, 0($t8)

        # update empty left
        addi    $s3, $s3, -1            # n*n - 1

        jr      $ra


###############################################
# print_board
#   Prints the current board:
#   for each row:
#     "+-" repeated n, then "+\n"
#     "|" then for each cell: glyph + "|", then "\n"
#   finally a bottom "+-...+\n"
#   Uses: $s0=n, $s1=board base
###############################################
print_board:
        # j = 0
        li      $t0, 0                  # t0 = j

# ---------- Row loop ----------
PB_RowLoop:
        bge     $t0, $s0, PB_AfterRows  # if (j >=n) break

        # print "+-" n times
        li      $t1, 0                  # t1 = i
PB_BarLoop:
        bge     $t1, $s0, PB_EndBar

        la      $a0, PLUSMIN            # "+-"
        li      $v0, 4
        syscall

        addiu   $t1, $t1, 1             # ++t1
        b       PB_BarLoop
PB_EndBar:
        li      $a0, '+'                 # print '+'
        li      $v0, 11
        syscall
        li      $a0, 10                # print 10
        li      $v0, 11
        syscall

        # print leading '|'
        li      $a0, '|'
        li      $v0, 11
        syscall

        # compute row pointer: &board[j*n]
        mul     $t2, $t0, $s0           # t2 = j*n
        add     $t2, $t2, $t2           # t2 = 2(j*n)
        add     $t2, $t2, $t2           # t2 = 4(j*n)
        addu    $t3, $s1, $t2           # t3 = board (row pointer)

        # inner cell loop: i = 0, ..., n-1
        li      $t1, 0                  # t1 = i
PB_CellLoop:
        bge     $t1, $s0, PB_EndCell

        # print cell and its glyph
        lw      $t4, 0($t3)             # cell = *board
        la      $t5, glyph              # table: [' ', 'O', 'X']
        addu    $t5, $t5, $t4           # &glyph[cell]
        lb      $a0, 0($t5)             # a0 = glyph char
        li      $v0, 11                 # print glyph
        syscall

        # print '|'
        li      $a0, '|'
        li      $v0, 11
        syscall

        addi    $t3, $t3, 4
        addi    $t1, $t1, 1
        b       PB_CellLoop
PB_EndCell:
        li      $a0, 10
        li      $v0, 11
        syscall

        addi    $t0, $t0, 1             # ++j
        b       PB_RowLoop

# ---------- After all rows ----------
PB_AfterRows:
        # bottom "+-...+"
        li      $t1, 0                  # t1 = i
PB_BotBarLoop:
        bge     $t1, $s0, PB_EndBotBar
        la      $a0, PLUSMIN
        li      $v0, 4
        syscall
        addi    $t1, $t1, 1
        b       PB_BotBarLoop
PB_EndBotBar:
        li      $a0, '+'
        li      $v0, 11
        syscall
        li      $a0, 10
        li      $v0, 11
        syscall

        jr      $ra                     # return

#########################################################
#
# data segment
#
#########################################################
        .data
WELCOME:        .asciiz "Let's play a game of tic-tac-toe.\nEnter n: "
FIRST:          .asciiz "I'll go first.\n"
PLUSMIN:        .asciiz "+-"
GETROW:         .asciiz "Enter row: "
GETCOL:         .asciiz "Enter col: "
INVALID:        .asciiz "Invalid move. Try again\n"
DRAW:           .asciiz "We have a draw!"
LOSE:           .asciiz "I'm the winner!"
WIN:            .asciiz "You are the winner!"
CPU_CHOOSE1:    .asciiz "I choose ("
CPU_COMMA:      .asciiz ", "
CPU_RPAREN:     .asciiz ")\n"
glyph:          .byte   ' ', 'O', 'X'   # maps cell 0/1/2 to ' '/O/X

# --- DEBUG strings ---
DBG_MM_RET:     .asciiz "MM ret="
DBG_RES:        .asciiz "RES="
DBG_IWM_N:      .asciiz "IWM n="
DBG_TURN:       .asciiz "TURN="
DBG_H:          .asciiz "H="
DBG_V:          .asciiz "V="
DBG_D1:         .asciiz "D1="
DBG_D2:         .asciiz "D2="
DBG_CD_TURN:    .asciiz "CD TURN="
DBG_N:          .asciiz "n="
DBG_BYTES:      .asciiz "bytes="
DBG_BOARD:      .asciiz "board="
DBG_CLR_END:    .asciiz "clr_end="
DBG_USER_TURN:  .asciiz "USER TURN="
DBG_CPU_START:  .asciiz "CPUMOVE turn="
DBG_CPU_WIN:    .asciiz "CPU WIN cand "
DBG_CPU_BLOCK:  .asciiz "CPU BLOCK cand "
DBG_CPU_FIRST:  .asciiz "CPU FIRST-AVAIL "
DBG_CPU_CHK:    .asciiz "CPU chk "
DBG_RET_EQ:     .asciiz " ret="
DBG_CPU_S1:     .asciiz " board_ptr="
DBG_CALL_MM:    .asciiz "CALL makemove "
DBG_IVM_R:      .asciiz "IVM r="
DBG_IVM_C:      .asciiz " c="
DBG_IVM_IDX:    .asciiz " idx="
DBG_IVM_ADDR:   .asciiz " addr="
DBG_IVM_CELL:   .asciiz " cell="

BOARD:          .word  0
# EOF (main.s)

