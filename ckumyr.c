/* myread.c by Kristoffer Eriksson, ske@pkmab.se, 21 Feb 1990. */

#ifdef MYREAD
/* Private buffer for myread() and it's companions. Not for use by anything
 * else. ttflui() is allowed to reset them to initial values. ttchk() is
 * allowed to read my_count.
 *
 * my_item is an index into mybuf[]. Increment it *before* reading mybuf[].
 *
 * A global parity mask variable could be useful too. We could use it to
 * let myread() strip the parity on it's own, in stead of stripping sign
 * bits as it does now.
 */

static CHAR mybuf[256];			/* Buffer, including push back */
static int my_count = 0;		/* Number of chars still in mybuf */
static int my_item = -1;		/* Last index read from mybuf[] */


/* myread() -- Efficient read of one character from communications line.
 *
 * Uses a private buffer to minimize the number of expensive read() system
 * calls. Essentially performs the equivalent of read() of 1 character, which
 * is then returned. By reading all available input from the system buffers
 * to the private buffer in one chunk, and then working from this buffer, the
 * number of system calls is reduced in any case where more than one character
 * arrives during the processing of the previous chunk, for instance high
 * baud rates or network type connections where input arrives in packets.
 * If the time needed for a read() system call approaches the time for more
 * than one character to arrive, then this mechanism automatically compensates
 * for that by performing bigger read()s less frequently. If the system load
 * is high, the same mechanism compensates for that too.
 *
 * myread() is a macro that returns the next character from the buffer. If the
 * buffer is empty, mygetbuf() is called. See mygetbuf() for possible error
 * returns.
 *
 * This should be efficient enough for any one-character-at-a-time loops.
 * For even better efficiency you might use memcpy()/bcopy() or such between
 * buffers (since they are often better optimized for copying), but it may not
 * be worth it if you have to take an extra pass over the buffer to strip
 * parity and check for CTRL-C anyway.
 *
 * Note that if you have been using myread() from another program module, you
 * may have some trouble accessing this macro version and the private variables
 * it uses. In that case, just add a function in this module, that invokes the
 * macro.
 */
#define myread()  (--my_count < 0 ? mygetbuf() : 255 & (int)mybuf[++my_item])


/* mygetbuf() -- Fill buffer for myread() and return first character.
 *
 * This function is what myread() uses when it can't get the next character
 * directly from it's buffer. Firstly, it calls a system dependant myfillbuf()
 * to read at least one new character into the buffer, and then it returns
 * the first character just as myread() would have done. This function also
 * is responsible for all error conditions that myread() can indicate.
 *
 * Returns: When OK => a positive character.
 *	    When EOF or error => -2.
 *
 * Older myread()s additionally returned -1 to indicate that there was nothing
 * to read, upon which the caller would call myread() again until it got
 * something. The new myread()/mygetbuf() always gets something. Any program
 * that actually depends on the old behaviour will break.
 *
 * Also sets errno to 9999 on EOF for compatibility with the old myread().
 * When ckucon is cleaned up to not depend on that, just remove this action
 * from mygetbuf().
 *
 * You could have mygetbuf() return different codes for EOF and other errors,
 * but kermit has currently no use for that. Most programs would just finish
 * up in both cases anyway.
 *
 * The debug() call maybe should be removed for optimum performance.
 */
mygetbuf() {
	my_count = myfillbuf();
	debug(F101, "myfillbuf read", "", my_count);
	if (my_count <= 0) {
		if (my_count == 0)
			errno = 9999;
		return -2;
	}
	--my_count;
	return(255 & (int)mybuf[my_item = 0]);
}


/* Specification: Push back up to one character onto myread()'s queue.
 *
 * This implementation: Push back characters into mybuf. At least one character
 * must have been read through myread() before myunrd() may be used. After
 * EOF or read error, again, myunrd() can not be used. Sometimes more than
 * one character can be pushed back, but only one character is guaranteed.
 * Since a previous myread() must have read it's character out of mybuf[],
 * that guarantees that there is space for at least one character. If push
 * back was really needed after EOF, a small addition could provide that.
 *
 * myunrd() is currently not called from anywhere inside kermit...
 */
myunrd(ch) CHAR ch; {
	if (my_item >= 0) {
		mybuf[my_item--] = ch;
		++my_count;
	}
}

/* myfillbuf(): System dependant read() into mybuf[], as many chars as possible.
 *
 * Returns: OK => number of characters read, always more than zero.
 *          EOF => 0
 *          Error => -1.
 *
 * If there is input available in the system's buffers, all of it should be
 * read inte mybuf[] and the function return immediately. If no input is
 * available, it should wait for a character to arrive, and return with that
 * one in mybuf[] as soon as possible. It may wait somewhat past the first
 * character, but be aware that any such delay lengthens the packet turnaround
 * time during kermit file transfers. Should never return with zero characters
 * unless EOF or irrecoverable read error.
 *
 * Correct functioning depends on the correct tty parameters being used.
 * Better control of current parameters is required than may have been the
 * case in kermit releases. For instance, O_NDELAY (or equivalent) can no
 * longer be sometimes off and sometimes on like it used to, unless a special
 * myfillbuf() is written to handle that. Otherwise the ordinary myfillbuf()s
 * may think they have come to EOF.
 *
 * If your system has a facility to direclty perform the functioning of
 * myfillbuf(), then use it. If the system can tell you have many characters
 * are available in it's buffers, then read that amount (but not less than 1).
 * If the system can return a special indication when you try to read without
 * anything to read, while allowing you to read all there is when there is
 * something, you may loop until there is something to read, but probably that
 * is not good for the system load.
 */

#ifdef UXIII
	/* This is for System III/V with VMIN>0, VTIME=0 and O_NDELAY off,
	 * and CLOCAL set any way you like. This way, read() will do exactly
	 * what is required by myfillbuf(): If there is data in the buffers
	 * of the O.S., all available data is read into mybuf, up to the size
	 * of mybuf. If there is none, the first character to arrive is
	 * awaited and returned.
	 */
myfillbuf() {
	return read(ttyfd, mybuf, sizeof(mybuf));
}

#else /* UXIII */


#ifdef aegis
	/* This is quoted from the old myread(). The semantics seem to be
	 * alright, but maybe errno would not need to be set even when
	 * there is no error? I don't know aegis.
	 */
myfillbuf() {
	int count;

	count = ios_$get((short)ttyfd, ios_$cond_opt, mybuf, 256L, st);
	errno = EIO;
	if (st.all == ios_$get_conditional_failed) /* get at least one */
		inbufc = ios_$get((short)ttyfd, 0, mybuf, 1L, st);
	if (st.all == ios_$end_of_file)
		return(0);
	else if (st.all != status_$ok) {
		errno = EIO;
		return(-1);
	}
	return(count);
}
#else /* aegis */


#ifdef FIONREAD
	/* This is for systems with FIONREAD. FIONREAD returns the number
	 * of characters available for reading. If none are available, wait
	 * until something arrives, otherwise return all there is.
	 */
myfillbuf() {
	long avail;

	if (ioctl(ttyfd, FIONREAD, &avail) < 0  ||  avail == 0)
		avail = 1;
	return read(ttyfd, mybuf, avail);
}

#else /* FIONREAD */
/* Add other systems here, between #ifdef and #else, e.g. NETCON. */

	/* When there is no other possibility, read 1 character at a time. */
myfillbuf() {
	return read(ttyfd, mybuf, 1);
}

#endif /* !FIONREAD */
#endif /* !aegis */
#endif /* !UXIII */

#endif /* MYREAD */
