const socket=io.connect('http://localhost:5000');

var textArray = []
var plotArray = []

var ids = ["temp", "humid", "co2"]

ids.forEach(function(id){
    textArray.push(document.getElementById(id+"Text"));
    plotArray.push(document.getElementById(id+"Plot"));
})

socket.on('update',function(message){
    const data = message.split(':');
    var i=0;
    for (; i<3; i++) {
        if(i!=0) textArray[i].innerHTML=data[i].substring(0, data[i].length - 2);
        else textArray[i].innerHTML=data[i];
        plotArray[i].src=`static/plots/plot${i}.png?a=`+Math.random();
    }
});