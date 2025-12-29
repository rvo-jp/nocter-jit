

let a = 0
let c
{
    let b = 1
    c = (p) => {
        console.log(a + p + b)
    }
}

a = 1

c(1)