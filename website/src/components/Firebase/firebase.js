import app from 'firebase/app';
import 'firebase/auth';
import 'firebase/database';

const config = {
    apiKey: process.env.REACT_APP_API_KEY,
    databaseURL: process.env.REACT_APP_DATABASE_URL,
    projectId: process.env.REACT_APP_PROJECT_ID,
    authDomain: process.env.REACT_APP_AUTH_DOMAIN,
    //storageBucket: process.env.REACT_APP_STORAGE_BUCKET,
    //messagingSenderId: process.env.REACT_APP_MESSAGING_SENDER_ID,
};

class Firebase {
    constructor() {
        app.initializeApp(config);
        this.auth = app.auth();
        this.db = app.database();
        this.login("lucas@luxaslabs.com", "foobar").then((user) => {
            console.log("logged in", user)
        })
    }

    login = (username, password) => { return this.auth.signInWithEmailAndPassword(username, password) }

    signOut = () => this.auth.signOut();

    fetchUsers = (callback) => {
        this.db.ref('users').on('value', (snapshot) => {
            const usersObj = snapshot.val()
            const users = []
            Object.keys(usersObj).forEach((key) => {
                const user = usersObj[key]
                user.userID = parseInt(key)
                if (!user.identifiers) {
                    user.identifiers = []
                }
                users.push(user)
            })
            callback(users)
        })
    }

    fetchRegistrations = (callback) => {
        this.db.ref('registrations').on("value", (snapshot) => {
            const registrations = []
            Object.entries(snapshot.val()).forEach(([key, value]) => {
                console.log(`${key} ${value}`); // "a 5", "b 7", "c 9"
                const parts = key.split("-")
                if (parts.length !== 2 || parts[1].length !== 8) {
                    // skip invalid key, should be of the 123456-20191125 form
                    return
                }
                const registration = value
                registration.userID = parseInt(parts[0])
                registration.dateString = parts[1]
                registrations.push(registration)
            })
            callback(registrations)
        })
    }
}
export default Firebase;