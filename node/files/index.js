const enemy = () => {
    if (Alpine.store('whoami') === 'red') return 'black'
    return 'red'
}

/**
 * Usuń pion jeżeli nastąpiło bicie podczas ruchu z `from` do `to`
 * @param from z kąd
 * @param to gdzie
 * @param who czyj pion zostanie zbity
 */
function usunZbity(from, to, who) {
    if (czyBicie(from, to)) {
        const filtered = Alpine
            .store(who)
            .filter(el => el !== bicie(from, to))
            .filter(el => !!el)
        Alpine.store(who, filtered)
    }
}

async function enemyMove() {
    try {
        const data = (await axios.get('/enemy')).data
        console.log('enemy move:', data);
        if (!data || data == '') return;
        if (data.from === '' || data.to === '') return;

        Alpine.store('kto', 'My turn')
        const { from, to } = data

        const oldBlacks = Alpine.store(enemy());
        const newBlacks = [...oldBlacks.filter(el => el !== from), to]
        Alpine.store(enemy(), newBlacks)

        usunZbity(from, to, Alpine.store('whoami'))
    }
    catch (error) {
        if (error.response) {
            if (error.response.status === 503 && error.response.data.status === 'closed') {
                Alpine.store('whoami', "surrender")
            }
        }
    }
}

function czyBicie(from, to) {
    const [fromx, fromy] = from.split('').map(el => Number(el));
    const [tox, toy] = to.split('').map(el => Number(el));
    console.log("[LOG CzyBicie]", { fromx, fromy }, { tox, toy });
    if (Math.abs(fromx - tox) == 2) {
        return true;
    }
    return false
}

async function acceptMove() {
    const from = Alpine.store('from');
    const to = Alpine.store('to');

    try {
        const res = (await axios.get('/status')).data.status

        console.log('response:', res);
        if (res === 'ACCEPT') {
            const oldBlacks = Alpine.store(Alpine.store('whoami'));
            let newBlacks = [...oldBlacks.filter(el => el !== from), to]
            Alpine.store(Alpine.store('whoami'), newBlacks)
            usunZbity(from, to, enemy())
            Alpine.store('kto', 'Enemy turn')
        }
        if (res === 'ERROR') {
            alert("Wrong move, try again")
        }
    }
    catch (error) {
        if (error.response) {
            if (error.response.status === 503 && error.response.data.status === 'closed') {
                Alpine.store('whoami', "surrender")
            }
        }
    }
    finally {
        Alpine.store('from', '');
        Alpine.store('to', '');
    }
}

function bicie(from, to) {
    const [fromx, fromy] = from.split('').map(el => Number(el));
    const [tox, toy] = to.split('').map(el => Number(el));
    console.log("[LOG Bicie]", { fromx, fromy }, { tox, toy });
    if (fromx + 2 == tox && fromy + 2 == toy) return `${fromx + 1}${fromy + 1}`;
    else if (fromx + 2 == tox && fromy - 2 == toy) return `${fromx + 1}${fromy - 1}`;
    else if (fromx - 2 == tox && fromy + 2 == toy) return `${fromx - 1}${fromy + 1}`;
    else if (fromx - 2 == tox && fromy - 2 == toy) return `${fromx - 1}${fromy - 1}`;
}

async function send() {
    const from = Alpine.store('from');
    const to = Alpine.store('to');
    if (from !== '' && to !== '' && from !== to) {

        try {
            await axios
                .post('/move', { from, to })
            await acceptMove();
        }
        catch (error) {
            if (error.response) {
                if (error.response.status === 503 && error.response.data.status === 'closed') {
                    Alpine.store('whoami', "surrender")
                }
            }
        }

    }
}

async function whoami() {
    try {
        const who = (await axios.get('/whoami')).data
        console.log({ who })
        Alpine.store('whoami', who.color)
        Alpine.store('kto', who.color === 'black' ? 'My turn' : 'Enemy turn')
    }
    catch (error) {
        if (error.response) {
            if (error.response.status === 503 && error.response.data.status === 'closed') {
                Alpine.store('whoami', "surrender")
            }
        }
    }
}

document.addEventListener('alpine:init', () => {
    Alpine.store('black', [
        '32', '34', '36', '38',
        '21', '23', '25', '27',
        '12', '14', '16', '18'
    ])
    Alpine.store('red', [
        '81', '83', '85', '87',
        '72', '74', '76', '78',
        '61', '63', '65', '67'
    ])
    Alpine.store('from', '')
    Alpine.store('to', '')
    Alpine.store('whoami', '')
    Alpine.store('kto', '')
})
