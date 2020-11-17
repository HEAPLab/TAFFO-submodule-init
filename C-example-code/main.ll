; ModuleID = 'main.c'
source_filename = "main.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.RgbPixel = type { float, float, float, i32, float }

@.str = private unnamed_addr constant [36 x i8] c"scalar(disabled range(-3000, 3000))\00", section "llvm.metadata"
@.str.1 = private unnamed_addr constant [7 x i8] c"main.c\00", section "llvm.metadata"
@.str.2 = private unnamed_addr constant [11 x i8] c"C1 is: %d\0A\00", align 1
@.str.3 = private unnamed_addr constant [11 x i8] c"C2 is: %d\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c1 = alloca i32, align 4
  %c2 = alloca i32, align 4
  %pixel = alloca %struct.RgbPixel, align 4
  %d = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  %a1 = bitcast i32* %a to i8*
  call void @llvm.var.annotation(i8* %a1, i8* getelementptr inbounds ([36 x i8], [36 x i8]* @.str, i32 0, i32 0), i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.1, i32 0, i32 0), i32 19)
  store i32 7, i32* %a, align 4
  store i32 5, i32* %b, align 4
  %0 = load i32, i32* %a, align 4
  store i32 %0, i32* %d, align 4
  %1 = load i32, i32* %a, align 4
  %2 = load i32, i32* %b, align 4
  %call = call i32 @foo1(i32 %1, i32 %2)
  store i32 %call, i32* %c1, align 4
  %3 = load i32, i32* %a, align 4
  %4 = load i32, i32* %b, align 4
  %call2 = call i32 @foo3(i32 %3, i32 %4)
  store i32 %call2, i32* %c2, align 4
  %5 = load i32, i32* %c1, align 4
  %call3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.2, i64 0, i64 0), i32 %5)
  %6 = load i32, i32* %c2, align 4
  %call4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @.str.3, i64 0, i64 0), i32 %6)
  ret i32 0
}

; Function Attrs: nounwind willreturn
declare void @llvm.var.annotation(i8*, i8*, i8*, i32) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @foo1(i32 %c, i32 %b) #0 {
entry:
  %c.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %a = alloca i32, align 4
  %d = alloca i32, align 4
  store i32 %c, i32* %c.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  store i32 1, i32* %a, align 4
  %0 = load i32, i32* %a, align 4
  store i32 %0, i32* %d, align 4
  %1 = load i32, i32* %c.addr, align 4
  %2 = load i32, i32* %b.addr, align 4
  %add = add nsw i32 %1, %2
  %3 = load i32, i32* %a, align 4
  %add1 = add nsw i32 %add, %3
  ret i32 %add1
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @foo3(i32 %a, i32 %b) #0 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %sub = sub nsw i32 %0, %1
  ret i32 %sub
}

declare dso_local i32 @printf(i8*, ...) #2

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind willreturn }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.1 (https://github.com/francescopont/llvm-project 913d9796eeeff3a1201e5c90b4468b9dfab680dd)"}
