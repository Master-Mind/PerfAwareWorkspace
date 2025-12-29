# PerfAwareWorkspace

This is a repo for the homework of Casey Muratori's Performance Aware Programming course.

# Rust usage

Was liking rust until I tried to use it for the haversine assignment, particularly the profiler. The profiler requires getting references to individual array elements, something which is apparently one of the single most difficult things you could possibly do in rust. 

This is due to the fact that the borrow checker operates at compile time, whereas array indexing happens at runtime. This means that borrowing a reference to one array element gets the borrow for the whole entire array, meaning that you can't get any more references to anything else in the array. This creates a whole host of problems when trying to operate on array elements, which is what I ran into while working on the profiler for haversine.

Because of that and because of the fact that this course will require using unsafe blocks everywhere anyways, means that I think I'll just use c++ for the rest of the course and use rust for a different project in the future.