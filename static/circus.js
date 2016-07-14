function submit_action(action) {
    document.action_form.action.value = action;
    document.action_form.submit();
}

function get_pass(key) {
    document.action_form.name = key;
    submit_action('pass');
}
