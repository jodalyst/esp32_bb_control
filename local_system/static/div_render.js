

function Div_Render(div_id,title,width,height,unique, socket=null){
    var div_id = div_id;
    var title = title;
    var unique = unique;
    var socket = socket;
    var width = width;
    var height = height;
    var div1 = document.createElement('div');
    var div2 = document.createElement('div');
    div1.className = "plot_title"
    div1.id = div_id+unique+"_title";
    div1.innerHTML = title;
    document.getElementById(div_id).appendChild(div1); 
    div2.id = div_id+unique+"_overall";
    document.getElementById(div_id).appendChild(div2); 
    if (socket != null){
        socket.on("update_"+unique,function(value){
        div2.innerHTML = value;
        evalScriptInHTML(div2);
        });

    }
};
function evalScriptInHTML(div) {
    var scripts = div.getElementsByTagName("script");
    for (var i = 0; i < scripts.length; ++i) {
        var script = scripts[i];
        eval(script.innerHTML);
    }
};

