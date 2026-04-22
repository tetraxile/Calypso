use std::io::Seek;

use num_traits::{PrimInt, Unsigned};

pub fn align_up<T: PrimInt + Unsigned>(val: T, align: T) -> T {
	align_up_checked(val, align).expect("attempt to align up with overflow")
}

pub fn align_up_checked<T: PrimInt + Unsigned>(val: T, align: T) -> Option<T> {
	debug_assert!(align.count_ones() == 1, "align must be a power of two");

	let val = (val.checked_add(&align)?.checked_sub(&T::one()))?;
	let mask = !(align.checked_sub(&T::one())?);

	Some(val & mask)
}

pub fn align_stream<T: Seek>(stream: &mut T, align: u64) -> std::io::Result<()> {
	let aligned_pos = align_up(stream.stream_position()?, align);
	stream.seek(std::io::SeekFrom::Start(aligned_pos))?;

	Ok(())
}
