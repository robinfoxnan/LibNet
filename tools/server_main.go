package main
import (
    "fmt"
    "net/http"
)
 
func IndexHandlers(w http.ResponseWriter, r *http.Request){

	// fmt.Println("============================")
	 id := r.Header["Pipeline-Id"][0]
	// if len(r.Header) > 0 {
    //   for k,v := range r.Header {
	// 	if (k == "Pipeline-Id"){
	// 	id = v[0]
	// 	}
    //      fmt.Printf("%s=%s\n", k, v[0])
    //   }
	// }
	// fmt.Println("");
    fmt.Fprintln(w, "hello, world =========> " + id)
}
 
func main (){
    http.HandleFunc("/", IndexHandlers)
	http.HandleFunc("/hi", IndexHandlers)
    err := http.ListenAndServe("10.128.6.15:80", nil)
    if err != nil {
        fmt.Printf("listen error:[%v]", err.Error())
    }
}

