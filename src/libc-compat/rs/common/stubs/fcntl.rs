// for platform those doesn't have O_PATH

#[cfg(mbb_stubs_O_PATH)]
pub const O_PATH: crate::c_int = 0;  // no-op
