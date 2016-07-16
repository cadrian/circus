function submit_action(action) {
    document.action_form.action.value = action;
    document.action_form.submit();
}

function get_pass(key) {
    document.action_form.key.value = key;
    submit_action('pass');
}

function tabulate(evt, tab) {
    var i, x;

    x = document.getElementsByClassName("tab");
    for (i = 0; i < x.length; i++) {
        x[i].style.display = "none";
    }
    document.getElementById(tab).style.display = "block";

    var e = evt || window.event;
    var target = e.target || e.currentTarget || e.srcElement;

    x = document.getElementsByClassName("tablink");
    for (i = 0; i < x.length; i++) {
        x[i].className = x[i].className.replace(" currenttab", "");
    }
    target.className += " currenttab";
}

document.addEventListener("DOMContentLoaded", function(event) {
    var i, x;

    x = document.getElementsByClassName("tab");
    x[0].style.display = "block";
    for (i = 1; i < x.length; i++) {
        x[i].style.display = "none";
    }

    x = document.getElementsByClassName("tablink");
    x[0].className += " currenttab";
});
