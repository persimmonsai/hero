vsim -voptargs="+acc" -t 1ps -warning 3009 hero_tb
set StdArithNoWarnings 1
set NumericStdNoWarnings 1
set BreakOnAssertion 2;# break also on assertion errors

if { ! [batch_mode] } {
    source ../test/hero_tb.wave.do
}

onfinish stop
run -a

# Set `quitCode` variable (to be used for the exit code) to `1` if the simulation terminated in an
# unknown state (e.g., due to a `$fatal`) and to `0` otherwise.
quietly set quitCode [expr [string match "*unknown" [runStatus -full]] ? 1 : 0]

# If the simulation terminated regularly, ..
if {! $quitCode } {
    quietly set resRegPath { /hero_tb/dut/i_periphs/i_soc_ctrl_regs/i_core_res/reg_q }
    # and the master core EOC'ed properly ..
    if { [examine -radix unsigned $resRegPath[0][31]] } {
        # .. return the value of the `main` function
        quietly set quitCode [examine -radix decimal $resRegPath[0][30:0]]
        printf "PULP execution returned %d." $quitCode
    } else {
        echo "Simulation finished without EOC from master core!"
        quietly set quitCode 1
    }
}