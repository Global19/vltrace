/*
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * trace_fs_path_1_3_arg_tmpl-sl.c -- Trace syscalls with numbers known from
 *    libc and filename as first argument. Single-packet version.
 *    Uses BCC, eBPF.
 */

/*
 * kprobe__SYSCALL_NAME -- SYSCALL_NAME() entry handler
 */
int
kprobe__SYSCALL_NAME(struct pt_regs *ctx)
{
	u64 pid_tid = bpf_get_current_pid_tgid();

	PID_CHECK_HOOK

	enum { _pad_size = offsetof(struct data_entry_t, aux_str) + NAME_MAX };

	union {
		struct data_entry_t ev;
		char _pad[_pad_size];
	} u;

	u.ev.type = E_SC_ENTRY;
	u.ev.start_ts_nsec = bpf_ktime_get_ns();

	u.ev.packet_type = 0; /* No additional packets */
	u.ev.sc_id = SYSCALL_NR; /* SysCall ID */
	u.ev.pid_tid = pid_tid;

	u.ev.arg_1 = PT_REGS_PARM1(ctx);
	u.ev.arg_2 = PT_REGS_PARM2(ctx);
	u.ev.arg_3 = PT_REGS_PARM3(ctx);
	u.ev.arg_4 = PT_REGS_PARM4(ctx);
	u.ev.arg_5 = PT_REGS_PARM5(ctx);
	u.ev.arg_6 = PT_REGS_PARM6(ctx);

	bpf_probe_read(&u.ev.aux_str, NAME_MAX / 2, (void *)u.ev.arg_1);
	bpf_probe_read((&u.ev.aux_str) + (NAME_MAX / 2),
		       NAME_MAX - (NAME_MAX / 2),
		       (void *)u.ev.arg_3);
	events.perf_submit(ctx, &u.ev, _pad_size);

	return 0;
};

/*
 * kretprobe__SYSCALL_NAME -- SYSCALL_NAME() exit handler
 */
int
kretprobe__SYSCALL_NAME(struct pt_regs *ctx)
{
	struct data_exit_t ev;

	u64 cur_nsec = bpf_ktime_get_ns();
	u64 pid_tid = bpf_get_current_pid_tgid();

	PID_CHECK_HOOK

	ev.type = E_SC_EXIT;
	ev.packet_type = 0; /* No additional packets */
	ev.sc_id = SYSCALL_NR; /* SysCall ID */
	ev.pid_tid = pid_tid;
	ev.finish_ts_nsec = cur_nsec;
	ev.ret = PT_REGS_RC(ctx);

	enum { ev_size = offsetof(struct data_exit_t, sc_name) };
	events.perf_submit(ctx, &ev, ev_size);

	return 0;
}
