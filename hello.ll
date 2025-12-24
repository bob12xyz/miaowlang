; ModuleID = 'miaow_module'
source_filename = "miaow_module"

@0 = private unnamed_addr constant [3 x i8] c"%d\00", align 1

define i32 @main() {
entry:
  %x = alloca i32, align 4
  %0 = alloca i32, align 4
  store i32 1, ptr %0, align 4
  %1 = load i32, ptr %0, align 4
  store i32 %1, ptr %x, align 4
  br label %cond

cond:                                             ; preds = %ifcont, %entry
  %2 = alloca i32, align 4
  store i32 100, ptr %2, align 4
  %3 = load i32, ptr %x, align 4
  %4 = load i32, ptr %2, align 4
  %5 = icmp sle i32 %3, %4
  %6 = alloca i1, align 1
  store i1 %5, ptr %6, align 1
  %whilecond = load i1, ptr %6, align 1
  br i1 %whilecond, label %loop, label %whilecont

loop:                                             ; preds = %cond
  br label %block

whilecont:                                        ; preds = %cond
  ret i32 0

block:                                            ; preds = %loop
  %7 = alloca i32, align 4
  store i32 15, ptr %7, align 4
  %8 = load i32, ptr %x, align 4
  %9 = load i32, ptr %7, align 4
  %10 = srem i32 %8, %9
  %11 = alloca i32, align 4
  store i32 %10, ptr %11, align 4
  %12 = alloca i32, align 4
  store i32 0, ptr %12, align 4
  %13 = load i32, ptr %11, align 4
  %14 = load i32, ptr %12, align 4
  %15 = icmp eq i32 %13, %14
  %16 = alloca i1, align 1
  store i1 %15, ptr %16, align 1
  %ifcond = load i1, ptr %16, align 1
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %block
  %str_struct = alloca { i32, i32, ptr }, align 8
  %size_ptr = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct, i32 0, i32 0
  store i32 8, ptr %size_ptr, align 4
  %cap_ptr = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct, i32 0, i32 1
  store i32 16, ptr %cap_ptr, align 4
  %str_data = alloca [9 x i8], align 1
  %char_ptr = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 0
  store i8 70, ptr %char_ptr, align 1
  %char_ptr1 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 1
  store i8 105, ptr %char_ptr1, align 1
  %char_ptr2 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 2
  store i8 122, ptr %char_ptr2, align 1
  %char_ptr3 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 3
  store i8 122, ptr %char_ptr3, align 1
  %char_ptr4 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 4
  store i8 66, ptr %char_ptr4, align 1
  %char_ptr5 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 5
  store i8 117, ptr %char_ptr5, align 1
  %char_ptr6 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 6
  store i8 122, ptr %char_ptr6, align 1
  %char_ptr7 = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 7
  store i8 122, ptr %char_ptr7, align 1
  %null_ptr = getelementptr inbounds [9 x i8], ptr %str_data, i32 0, i32 8
  store i8 0, ptr %null_ptr, align 1
  %data_ptr_ptr = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct, i32 0, i32 2
  store ptr %str_data, ptr %data_ptr_ptr, align 8
  %str_ref = alloca ptr, align 8
  store ptr %str_struct, ptr %str_ref, align 8
  %str_ptr = load ptr, ptr %str_ref, align 8
  %data_ptr_ptr8 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_ptr, i32 0, i32 2
  %data_ptr = load ptr, ptr %data_ptr_ptr8, align 8
  %17 = call i32 @puts(ptr %data_ptr)
  br label %ifcont

else:                                             ; preds = %block
  %18 = alloca i32, align 4
  store i32 3, ptr %18, align 4
  %19 = load i32, ptr %x, align 4
  %20 = load i32, ptr %18, align 4
  %21 = srem i32 %19, %20
  %22 = alloca i32, align 4
  store i32 %21, ptr %22, align 4
  %23 = alloca i32, align 4
  store i32 0, ptr %23, align 4
  %24 = load i32, ptr %22, align 4
  %25 = load i32, ptr %23, align 4
  %26 = icmp eq i32 %24, %25
  %27 = alloca i1, align 1
  store i1 %26, ptr %27, align 1
  %ifcond9 = load i1, ptr %27, align 1
  br i1 %ifcond9, label %then10, label %else11

ifcont:                                           ; preds = %ifcont12, %then
  br label %cond

then10:                                           ; preds = %else
  %str_struct13 = alloca { i32, i32, ptr }, align 8
  %size_ptr14 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct13, i32 0, i32 0
  store i32 4, ptr %size_ptr14, align 4
  %cap_ptr15 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct13, i32 0, i32 1
  store i32 8, ptr %cap_ptr15, align 4
  %str_data16 = alloca [5 x i8], align 1
  %char_ptr17 = getelementptr inbounds [5 x i8], ptr %str_data16, i32 0, i32 0
  store i8 70, ptr %char_ptr17, align 1
  %char_ptr18 = getelementptr inbounds [5 x i8], ptr %str_data16, i32 0, i32 1
  store i8 105, ptr %char_ptr18, align 1
  %char_ptr19 = getelementptr inbounds [5 x i8], ptr %str_data16, i32 0, i32 2
  store i8 122, ptr %char_ptr19, align 1
  %char_ptr20 = getelementptr inbounds [5 x i8], ptr %str_data16, i32 0, i32 3
  store i8 122, ptr %char_ptr20, align 1
  %null_ptr21 = getelementptr inbounds [5 x i8], ptr %str_data16, i32 0, i32 4
  store i8 0, ptr %null_ptr21, align 1
  %data_ptr_ptr22 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct13, i32 0, i32 2
  store ptr %str_data16, ptr %data_ptr_ptr22, align 8
  %str_ref23 = alloca ptr, align 8
  store ptr %str_struct13, ptr %str_ref23, align 8
  %str_ptr24 = load ptr, ptr %str_ref23, align 8
  %data_ptr_ptr25 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_ptr24, i32 0, i32 2
  %data_ptr26 = load ptr, ptr %data_ptr_ptr25, align 8
  %28 = call i32 @puts(ptr %data_ptr26)
  br label %ifcont12

else11:                                           ; preds = %else
  %29 = alloca i32, align 4
  store i32 5, ptr %29, align 4
  %30 = load i32, ptr %x, align 4
  %31 = load i32, ptr %29, align 4
  %32 = srem i32 %30, %31
  %33 = alloca i32, align 4
  store i32 %32, ptr %33, align 4
  %34 = alloca i32, align 4
  store i32 0, ptr %34, align 4
  %35 = load i32, ptr %33, align 4
  %36 = load i32, ptr %34, align 4
  %37 = icmp eq i32 %35, %36
  %38 = alloca i1, align 1
  store i1 %37, ptr %38, align 1
  %ifcond27 = load i1, ptr %38, align 1
  br i1 %ifcond27, label %then28, label %else29

ifcont12:                                         ; preds = %ifcont30, %then10
  br label %ifcont

then28:                                           ; preds = %else11
  %str_struct31 = alloca { i32, i32, ptr }, align 8
  %size_ptr32 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct31, i32 0, i32 0
  store i32 4, ptr %size_ptr32, align 4
  %cap_ptr33 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct31, i32 0, i32 1
  store i32 8, ptr %cap_ptr33, align 4
  %str_data34 = alloca [5 x i8], align 1
  %char_ptr35 = getelementptr inbounds [5 x i8], ptr %str_data34, i32 0, i32 0
  store i8 66, ptr %char_ptr35, align 1
  %char_ptr36 = getelementptr inbounds [5 x i8], ptr %str_data34, i32 0, i32 1
  store i8 117, ptr %char_ptr36, align 1
  %char_ptr37 = getelementptr inbounds [5 x i8], ptr %str_data34, i32 0, i32 2
  store i8 122, ptr %char_ptr37, align 1
  %char_ptr38 = getelementptr inbounds [5 x i8], ptr %str_data34, i32 0, i32 3
  store i8 122, ptr %char_ptr38, align 1
  %null_ptr39 = getelementptr inbounds [5 x i8], ptr %str_data34, i32 0, i32 4
  store i8 0, ptr %null_ptr39, align 1
  %data_ptr_ptr40 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_struct31, i32 0, i32 2
  store ptr %str_data34, ptr %data_ptr_ptr40, align 8
  %str_ref41 = alloca ptr, align 8
  store ptr %str_struct31, ptr %str_ref41, align 8
  %str_ptr42 = load ptr, ptr %str_ref41, align 8
  %data_ptr_ptr43 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_ptr42, i32 0, i32 2
  %data_ptr44 = load ptr, ptr %data_ptr_ptr43, align 8
  %39 = call i32 @puts(ptr %data_ptr44)
  br label %ifcont30

else29:                                           ; preds = %else11
  %40 = load i32, ptr %x, align 4
  %41 = alloca [12 x i8], align 1
  %42 = call i32 (ptr, ptr, ...) @sprintf(ptr %41, ptr @0, i32 %40)
  %conv_str_struct = alloca { i32, i32, ptr }, align 8
  %size_ptr45 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %conv_str_struct, i32 0, i32 0
  store i32 %42, ptr %size_ptr45, align 4
  %cap_ptr46 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %conv_str_struct, i32 0, i32 1
  store i32 12, ptr %cap_ptr46, align 4
  %data_ptr_ptr47 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %conv_str_struct, i32 0, i32 2
  store ptr %41, ptr %data_ptr_ptr47, align 8
  %str_ref48 = alloca ptr, align 8
  store ptr %conv_str_struct, ptr %str_ref48, align 8
  %str_ptr49 = load ptr, ptr %str_ref48, align 8
  %data_ptr_ptr50 = getelementptr inbounds nuw { i32, i32, ptr }, ptr %str_ptr49, i32 0, i32 2
  %data_ptr51 = load ptr, ptr %data_ptr_ptr50, align 8
  %43 = call i32 @puts(ptr %data_ptr51)
  br label %ifcont30

ifcont30:                                         ; preds = %else29, %then28
  br label %ifcont12
}

declare i32 @puts(ptr)

declare i32 @sprintf(ptr, ptr, ...)
