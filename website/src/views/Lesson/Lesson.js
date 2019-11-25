import React, {useState, useEffect} from "react";
// @material-ui/core components
import { makeStyles } from "@material-ui/core/styles";
// core components
import GridItem from "components/Grid/GridItem.js";
import GridContainer from "components/Grid/GridContainer.js";
import Table from "components/Table/Table.js";
import Card from "components/Card/Card.js";
import CardHeader from "components/Card/CardHeader.js";
import CardBody from "components/Card/CardBody.js";
import Tasks from "components/Tasks/Tasks.js";
import CardFooter from "components/Card/CardFooter.js";
import UpdateIcon from "@material-ui/icons/Update";
import CardIcon from "components/Card/CardIcon.js";
import Accessibility from "@material-ui/icons/Accessibility";
import importedstyles from "assets/jss/material-dashboard-react/views/dashboardStyle.js";

import {withFirebase} from '../../components/Firebase';
import { formatYYYYDDMM, dateFromTimestamp } from "../../util";

const styles = Object.assign(importedstyles, {
  cardCategoryWhite: {
    "&,& a,& a:hover,& a:focus": {
      color: "rgba(255,255,255,.62)",
      margin: "0",
      fontSize: "14px",
      marginTop: "0",
      marginBottom: "0"
    },
    "& a,& a:hover,& a:focus": {
      color: "#FFFFFF"
    }
  },
  cardTitleWhite: {
    color: "#FFFFFF",
    marginTop: "0px",
    minHeight: "auto",
    fontWeight: "300",
    fontFamily: "'Roboto', 'Helvetica', 'Arial', sans-serif",
    marginBottom: "3px",
    textDecoration: "none",
    "& small": {
      color: "#777",
      fontSize: "65%",
      fontWeight: "400",
      lineHeight: "1"
    }
  }
});
const useStyles = makeStyles(styles)

export default withFirebase((props) => {
  const [users, setUsers] = useState({})
  const [taskIndexes, setTaskIndexes] = useState([])
  const [checkedIndexes, setCheckedIndexes] = useState([])
  const classes = useStyles()
  const { firebase, location } = props;
  console.log("location", location)
  const lessonDate = location.pathname.split("/")[3]

  useEffect(() => {
    const taskIndexes = []
    firebase.fetchUsers((users) => {
      const userNames = {}
      users.forEach((val) => {
        taskIndexes.push(val.userID)
        userNames[val.userID] = val.firstName + " " + val.lastName;
      })

      firebase.fetchRegistrations((registrations) => {
        console.log("registrations", registrations)
        // https://stackoverflow.com/questions/24806772/how-to-skip-over-an-element-in-map
        const checkedIndexes = registrations.reduce(function(result, registration) {
          //var datestr = formatYYYYDDMM(dateFromTimestamp(registration.timestamp))
          //console.log(datestr)
          if (registration.dateString === lessonDate) {
            //const user = users.find((user) => {
            //  return user.userID === registration.userID.toString()
            //})
            console.log("found user index", users, registration.userID)
            result.push(registration.userID)
          }
          return result;
        }, []);
  
        console.log(userNames)
        console.log(taskIndexes)
        console.log(checkedIndexes)
    
        setUsers(userNames)
        setCheckedIndexes(checkedIndexes)
        setTaskIndexes(taskIndexes)
      })
    })
    return function cleanup() {
      setUsers([])
      setTaskIndexes([])
      setCheckedIndexes([])
    }
  }, [firebase, lessonDate])

  const onCheckedChanged = (userID, value) => {
    console.log("oncheckedchanged", userID, value)
    if (value) {
      firebase.db.ref(`registrations/${userID}-${lessonDate}`).set({
        timestamp: Math.floor(Date.now() / 1000),
        device: "web-ui",
      })
    } else {
      firebase.db.ref(`registrations/${userID}-${lessonDate}`).remove()
    }
  }

  return (
    <div>
      <GridContainer>
    <GridItem xs={12} sm={12} md={12}>
      <Card>
        <CardHeader color="primary">
          <h4 className={classes.cardTitleWhite}>Lesson {lessonDate}</h4>
          <p className={classes.cardCategoryWhite}>
            Lesson description
          </p>
        </CardHeader>
        <CardBody>
        <Tasks
          checkedIndexes={checkedIndexes}
          tasksIndexes={taskIndexes}
          tasks={users}
          onCheckedChanged={(index, value) => onCheckedChanged(index, value)} />
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
              <CardHeader color="info" stats icon>
              <CardIcon color="info">
              <Accessibility />
              </CardIcon>
              <p className={classes.cardCategory}>Followers</p>
              <h3 className={classes.cardTitle}>+245</h3>
              </CardHeader>
              <CardFooter stats>
              <div className={classes.stats}>
                  <UpdateIcon />
                  Just Updated
              </div>
              </CardFooter>
          </Card>
          </GridItem>
      </GridContainer>
*/