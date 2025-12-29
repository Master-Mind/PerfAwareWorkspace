use std::arch::x86_64::_rdtsc;
use std::rc::Rc;
use simd_json::prelude::IndexedMut;

pub struct Profiler<'a> {
    pub(crate) cur_traces: Vec<usize>,
    pub(crate) trace_anchors: Vec<TraceAnchor<'a>>
}

impl<'a> Profiler<'a> {
    #[track_caller]
    pub unsafe fn block_trace(&'a mut self, label: &'static str) -> Trace<'a> {
        let idx = std::panic::Location::caller().line() as usize;
        let anchor : *mut TraceAnchor = &mut self.trace_anchors[idx];
        (*anchor).label = label;

        if !self.cur_traces.is_empty() {
            let parentIDX = self.cur_traces[self.cur_traces.len() - 1];
            (*anchor).parent = Some(&self.trace_anchors[parentIDX]);
        }

        self.cur_traces.push(idx);

        Trace { start: unsafe {
            _rdtsc()
        }, anchor }
    }

    pub fn print_stats(&mut self) {
        for i in 0..self.trace_anchors.len() {
            if self.trace_anchors[i].num_elapsed > 0 {
                println!("{0}: {1}",self.trace_anchors[i].label, self.trace_anchors[i].total_elapsed);
            }
        }
    }
}

#[derive(Copy)]
#[derive(Clone)]
pub struct TraceAnchor<'a> {
    pub(crate) total_elapsed: u64,
    pub(crate) num_elapsed: u64,
    pub(crate) label: &'static str,
    pub(crate) parent: Option<&'a TraceAnchor<'a>>
}

pub struct Trace<'a> {
    start: u64,
    anchor: *mut TraceAnchor<'a>
}

impl<'a> Drop for Trace<'a> {
    fn drop(&mut self) {
        unsafe {
            (*self.anchor).total_elapsed += _rdtsc() - self.start;
            (*self.anchor).num_elapsed += 1;
        }
    }
}