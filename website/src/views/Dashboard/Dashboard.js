import React, {useState, useEffect} from "react";
// @material-ui/core
import { makeStyles } from "@material-ui/core/styles";
// core components
import GridItem from "components/Grid/GridItem.js";
import GridContainer from "components/Grid/GridContainer.js";
import Table from "components/Table/Table.js";
import Card from "components/Card/Card.js";
import CardHeader from "components/Card/CardHeader.js";
import CardBody from "components/Card/CardBody.js";
import Button from "components/CustomButtons/Button.js";
import CustomInput from "components/CustomInput/CustomInput.js";

import {withFirebase} from '../../components/Firebase';
import {formatDate, dateFromTimestamp} from '../../util'

import styles from "assets/jss/material-dashboard-react/views/dashboardStyle.js";

/* 
Very useful article:
* https://www.robinwieruch.de/complete-firebase-authentication-react-tutorial#manage-users-with-firebases-realtime-database-in-react
*/

const useStyles = makeStyles(styles);

export default withFirebase((props) => {
  const classes = useStyles();

  const { firebase } = props;
  const [users, setUsers] = useState([])
  const [newCards, setNewCards] = useState([])
  const [newUserID, setNewUserID] = useState("")
  const [newUserFirst, setNewUserFirst] = useState("")
  const [newUserLast, setNewUserLast] = useState("")

  useEffect(() => {
    firebase.fetchUsers((users) => {
      const mappedUsers = users.map((val) => {
        return [val.userID, val.firstName + " " + val.lastName, val.identifiers.join(" ")];
      })
      console.log(mappedUsers)
  
      setUsers(mappedUsers)
    })
    return function cleanup() {
      setUsers([])
    }
  }, [firebase])

  useEffect(() => {
    firebase.db.ref('newCards').on('value', (snapshot) => {
      if (!snapshot.val()) {
        setNewCards([])
        return
      }
      const mappedCards = Object.values(snapshot.val()).map((val) => {
        return [formatDate(dateFromTimestamp(val.timestamp)), val.identifier];
      })
      console.log(mappedCards)

      setNewCards(mappedCards)
    })
    
    return function cleanup() {
      setNewCards([])
    }
  }, [firebase])

  const addUser = () => {
    if (!newUserID) {
      console.log("newUserID was empty", newUserID)
      return
    }
    console.log(newUserID, newUserFirst, newUserLast)
    firebase.db.ref(`users/${newUserID}`).set({
      userID: parseInt(newUserID),
      firstName: newUserFirst,
      lastName: newUserLast,
      identifiers: [],
    })
    users.push([newUserID, newUserFirst + " " + newUserLast, ""])
    setNewUserID("")
    setNewUserFirst("")
    setNewUserLast("")
    setUsers(users)
  }

  const onNewCardClick = (key) => {
    console.log("onNewCardClick", key)
    firebase.db.ref('newCards').once('value').then((snapshot) => {
      var removeKey = ""
      Object.entries(snapshot.val()).forEach(([id,value]) => {
        if (value.identifier === newCards[key][1]) {
          removeKey = id
        }
      })
      firebase.db.ref(`newCards/${removeKey}`).remove()
    })
  }
  const onUserClick = (key) => {
    console.log("onUserClick", key)
    const userID = users[key][0]
    var identifiers = users[key][2].split(" ")
    if (identifiers[0] === "") {
      identifiers = []
    }
    identifiers.push(newCards[0][1])
    firebase.db.ref(`users/${userID}/identifiers`).set(identifiers)

    // remove the top new card
    onNewCardClick(0)
  }

  return (
    <div>
    <GridContainer>
      <GridItem xs={12} sm={12} md={6}>
        <Card>
          <CardHeader color="warning">
            <h4 className={classes.cardTitleWhite}>Students</h4>
            <p className={classes.cardCategoryWhite}>
              Known Students and their identifiers
            </p>
          </CardHeader>
          <CardBody>
            <Table
              tableHeaderColor="warning"
              tableHead={["ID", "Name", "Identifiers"]}
              tableData={users}
              onRowClick={(key) => onUserClick(key)}
            />
            <GridContainer alignItems="flex-end">
              <GridItem xs={2}>
                <CustomInput
                  labelText="User ID"
                  id="userID"
                  formControlProps={{
                    fullWidth: true
                  }}
                  inputProps={{
                    onChange: (ev) => setNewUserID(ev.target.value),
                    value: newUserID
                  }}
                />
              </GridItem>
              <GridItem xs={3}>
                <CustomInput
                  labelText="First Name"
                  id="firstName"
                  formControlProps={{
                    fullWidth: true
                  }}
                  inputProps={{
                    onChange: (ev) => setNewUserFirst(ev.target.value),
                    value: newUserFirst
                  }}
                />
              </GridItem>
              <GridItem xs={3}>
                <CustomInput
                  labelText="Last Name"
                  id="lastName"
                  formControlProps={{
                    fullWidth: true
                  }}
                  inputProps={{
                    onChange: (ev) => setNewUserLast(ev.target.value),
                    value: newUserLast
                  }}
                />
              </GridItem>
              <GridItem xs={4}>
                <Button color="primary" onClick={() => addUser()}>
                  Add New User
                </Button>
              </GridItem>
            </GridContainer>
          </CardBody>
        </Card>
      </GridItem>
      <GridItem xs={12} sm={12} md={6}>
        <Card>
          <CardHeader color="warning">
            <h4 className={classes.cardTitleWhite}>Newly Registered Cards</h4>
            <p className={classes.cardCategoryWhite}>
              Known Students and their identifiers
            </p>
          </CardHeader>
          <CardBody>
            <Table
              tableHeaderColor="warning"
              tableHead={["Time", "Identifier"]}
              tableData={newCards}
              onRowClick={(key) => onNewCardClick(key)}
            />
          </CardBody>
        </Card>
      </GridItem>
    </GridContainer>
  </div>
  );
})

/*
<GridContainer>
      <GridItem xs={12} sm={6} md={3}>
        <Card>
          <CardHeader color="warning" stats icon>
            <CardIcon color="warning">
              <Icon>content_copy</Icon>
            </CardIcon>
            <p className={classes.cardCategory}>Used Space</p>
            <h3 className={classes.cardTitle}>
              49/50 <small>GB</small>
            </h3>
          </CardHeader>
          <CardFooter stats>
            <div className={classes.stats}>
              <Danger>
                <Warning />
              </Danger>
              <a href="#pablo" onClick={e => e.preventDefault()}>
                Get more space
              </a>
            </div>
          </CardFooter>
        </Card>
      </GridItem>
      <GridItem xs={12} sm={6} md={3}>
        <Card>
          <CardHeader color="success" stats icon>
            <CardIcon color="success">
              <Store />
            </CardIcon>
            <p className={classes.cardCategory}>Revenue</p>
            <h3 className={classes.cardTitle}>$34,245</h3>
          </CardHeader>
          <CardFooter stats>
            <div className={classes.stats}>
              <DateRange />
              Last 24 Hours
            </div>
          </CardFooter>
        </Card>
      </GridItem>
      <GridItem xs={12} sm={6} md={3}>
        <Card>
          <CardHeader color="danger" stats icon>
            <CardIcon color="danger">
              <Icon>info_outline</Icon>
            </CardIcon>
            <p className={classes.cardCategory}>Fixed Issues</p>
            <h3 className={classes.cardTitle}>75</h3>
          </CardHeader>
          <CardFooter stats>
            <div className={classes.stats}>
              <LocalOffer />
              Tracked from Github
            </div>
          </CardFooter>
        </Card>
      </GridItem>
      <GridItem xs={12} sm={6} md={3}>
        <Card>
          <CardHeader color="info" stats icon>
            <CardIcon color="info">
              <Accessibility />
            </CardIcon>
            <p className={classes.cardCategory}>Followers</p>
            <h3 className={classes.cardTitle}>+245</h3>
          </CardHeader>
          <CardFooter stats>
            <div className={classes.stats}>
              <Update />
              Just Updated
            </div>
          </CardFooter>
        </Card>
      </GridItem>
    </GridContainer>
<GridContainer>
      <GridItem xs={12} sm={12} md={4}>
        <Card chart>
          <CardHeader color="success">
            <ChartistGraph
              className="ct-chart"
              data={dailySalesChart.data}
              type="Line"
              options={dailySalesChart.options}
              listener={dailySalesChart.animation}
            />
          </CardHeader>
          <CardBody>
            <h4 className={classes.cardTitle}>Daily Sales</h4>
            <p className={classes.cardCategory}>
              <span className={classes.successText}>
                <ArrowUpward className={classes.upArrowCardCategory} /> 55%
              </span>{" "}
              increase in today sales.
            </p>
          </CardBody>
          <CardFooter chart>
            <div className={classes.stats}>
              <AccessTime /> updated 4 minutes ago
            </div>
          </CardFooter>
        </Card>
      </GridItem>
      <GridItem xs={12} sm={12} md={4}>
        <Card chart>
          <CardHeader color="warning">
            <ChartistGraph
              className="ct-chart"
              data={emailsSubscriptionChart.data}
              type="Bar"
              options={emailsSubscriptionChart.options}
              responsiveOptions={emailsSubscriptionChart.responsiveOptions}
              listener={emailsSubscriptionChart.animation}
            />
          </CardHeader>
          <CardBody>
            <h4 className={classes.cardTitle}>Email Subscriptions</h4>
            <p className={classes.cardCategory}>Last Campaign Performance</p>
          </CardBody>
          <CardFooter chart>
            <div className={classes.stats}>
              <AccessTime /> campaign sent 2 days ago
            </div>
          </CardFooter>
        </Card>
      </GridItem>
      <GridItem xs={12} sm={12} md={4}>
        <Card chart>
          <CardHeader color="danger">
            <ChartistGraph
              className="ct-chart"
              data={completedTasksChart.data}
              type="Line"
              options={completedTasksChart.options}
              listener={completedTasksChart.animation}
            />
          </CardHeader>
          <CardBody>
            <h4 className={classes.cardTitle}>Completed Tasks</h4>
            <p className={classes.cardCategory}>Last Campaign Performance</p>
          </CardBody>
          <CardFooter chart>
            <div className={classes.stats}>
              <AccessTime /> campaign sent 2 days ago
            </div>
          </CardFooter>
        </Card>
      </GridItem>
    </GridContainer>
<GridItem xs={12} sm={12} md={6}>
  <CustomTabs
    title="Tasks:"
    headerColor="primary"
    tabs={[
      {
        tabName: "Bugs",
        tabIcon: BugReport,
        tabContent: (
          <Tasks
            checkedIndexes={[0, 3]}
            tasksIndexes={[0, 1, 2, 3]}
            tasks={bugs}
          />
        )
      },
      {
        tabName: "Website",
        tabIcon: Code,
        tabContent: (
          <Tasks
            checkedIndexes={[0]}
            tasksIndexes={[0, 1]}
            tasks={website}
          />
        )
      },
      {
        tabName: "Server",
        tabIcon: Cloud,
        tabContent: (
          <Tasks
            checkedIndexes={[1]}
            tasksIndexes={[0, 1, 2]}
            tasks={server}
          />
        )
      }
    ]}
  />
</GridItem>*/