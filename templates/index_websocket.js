const socket=io.connect('http://localhost:5001');

var textArray = []
var plotArray = []

var ids = ["temp", "humid", "co2"]

ids.forEach(function(id){
    textArray.push(document.getElementById(id+"Text"));
    plotArray.push(document.getElementById(id+"Plot"));
})

socket.on('update',function(message){
    const data = message.split(':');
    for (int i=0; i<3; i++) {
        textArray[i].innerHTML=data[i];
        plotArray[i].src=`images/plot${i}.png?a=`+Math.random();
    }
});
function update() {
    socket.emit('get_data'); 
}

setInterval(update, 1000);